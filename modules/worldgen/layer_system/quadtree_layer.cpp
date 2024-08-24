#include "quadtree_layer.h"
#include "scene/3d/node_3d.h"
#include "terrain_layers.h"
#include "core/config/project_settings.h"
#include "worldgen/instance_texture_queue.h"

QuadTreeTerrainLayer::QuadTreeTerrainLayer(Ref<RoadLayer> p_road_layer) {
    road_layer = p_road_layer;
    quad_tree_settings.instantiate();
    terrain_material = ResourceLoader::load(GLOBAL_GET("kgame/terrain/terrain_material"));
    int tjunction_permutations[9] = {
        (PlaneGenerate::GridTJunctionRemovalFlags)0,
            PlaneGenerate::GridTJunctionRemovalFlags::UP,
            PlaneGenerate::GridTJunctionRemovalFlags::DOWN,
            PlaneGenerate::GridTJunctionRemovalFlags::LEFT,
            PlaneGenerate::GridTJunctionRemovalFlags::RIGHT,
            PlaneGenerate::GridTJunctionRemovalFlags::UP | PlaneGenerate::GridTJunctionRemovalFlags::RIGHT,
            PlaneGenerate::GridTJunctionRemovalFlags::DOWN | PlaneGenerate::GridTJunctionRemovalFlags::LEFT,
            PlaneGenerate::GridTJunctionRemovalFlags::LEFT | PlaneGenerate::GridTJunctionRemovalFlags::UP,
            PlaneGenerate::GridTJunctionRemovalFlags::RIGHT | PlaneGenerate::GridTJunctionRemovalFlags::DOWN,
    };

    PlaneGenerate::GridMeshSettings mesh_settings = {
        .element_count = 32,
        .side_length = 1.0,  
    };

    for (int perm : tjunction_permutations) {
        PlaneGenerate::GridMesh mesh;
        PlaneGenerate::generate_mesh(mesh_settings, perm, mesh);
        Array mesh_arr;
        mesh_arr.resize(RS::ARRAY_MAX);
        mesh_arr[RS::ARRAY_INDEX] = mesh.indices;
        mesh_arr[RS::ARRAY_VERTEX] = mesh.positions;
        mesh_arr[RS::ARRAY_TEX_UV] = mesh.uvs;
        mesh.normals.resize(mesh.positions.size());
        mesh.normals.fill(Vector3(0.0f, 1.0f, 0.0f));
        mesh_arr[RS::ARRAY_NORMAL] = mesh.normals;

        Ref<ArrayMesh> gpu_mesh;
        gpu_mesh.instantiate();
        gpu_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mesh_arr);
        gpu_mesh->surface_set_material(0, terrain_material);
        grid_meshes.insert(perm, gpu_mesh);
    }
}

Ref<ChunkerChunk> QuadTreeTerrainLayer::create_chunk() const {
    Ref<QuadTreeTerrainChunk> chunk;
    chunk.instantiate(const_cast<QuadTreeTerrainLayer*>(this));
    return chunk;
}

Vector2 QuadTreeTerrainLayer::get_camera_position() const { return camera_position; }

void QuadTreeTerrainLayer::set_camera_position(const Vector2 &p_camera_position) { camera_position = p_camera_position; }

void QuadTreeTerrainLayer::update_terrain_chunks() {
    for (KeyValue<Vector2i, Ref<ChunkerChunk>> kv : loaded_chunks) {
        Ref<QuadTreeTerrainChunk> terrain_chunk = kv.value;
        terrain_chunk->camera_position = get_camera_position();
        terrain_chunk->update_quadtree();
        terrain_chunk->update_mesh_instances();
    }
}

QuadTreeTerrainChunk::QuadTreeTerrainChunk(QuadTreeTerrainLayer *p_layer) {
    layer = p_layer;
    quad_tree.instantiate();
    quad_tree->set_settings(layer->quad_tree_settings);
}

void QuadTreeTerrainChunk::update_quadtree() {
    quad_tree->set_bounds(bounds);
    quad_tree->clear();
    quad_tree->insert_camera(camera_position);
}

void QuadTreeTerrainChunk::update_mesh_instances() {
    if (!chunk_node) {
        chunk_node = memnew(Node3D);
        layer->get_manager()->add_child(chunk_node);
        chunk_node->set_name(vformat("%dx%d", chunk.x, chunk.y));
    }

    // Find which grid nodes we have, and see if we need to swap the mesh
    Vector<ChunkerQuadTree::LeafNodeInfo> node_infos = quad_tree->get_leaf_node_infos();
    HashSet<Rect2> node_bounds;
    Vector<int> nodes_to_create;
    for (int i = 0; i < node_infos.size(); i++) {
        node_bounds.insert(node_infos[i].bounds);
        HashMap<Rect2, GridNode>::Iterator it = loaded_grid_nodes.find(node_infos[i].bounds);
        if (it == loaded_grid_nodes.end()) {
            nodes_to_create.push_back(i);
            continue;
        }

        // We already have this node, see if we have to change the mesh due to LOD differences
        Ref<Mesh> new_mesh = get_mesh_for_lods(node_infos[i].lod_level, node_infos[i].neighbor_lods);
        if (new_mesh != it->value.mi->get_mesh()) {
            it->value.mi->set_mesh(new_mesh);
        }
    }

    // Unload me nodes
    LocalVector<Rect2> nodes_to_delete;
    for (KeyValue<Rect2, GridNode> chunk : loaded_grid_nodes) {
        if (!node_bounds.has(chunk.key)) {
            nodes_to_delete.push_back(chunk.key);
        }
    }

    for (Rect2 bounds : nodes_to_delete) {
        unload_grid_node(loaded_grid_nodes[bounds]);
        loaded_grid_nodes.erase(bounds);
    }

    // Create new ones
    for (int node_i : nodes_to_create) {
        const ChunkerQuadTree::LeafNodeInfo &node_info = node_infos[node_i];
        Ref<Mesh> new_mesh = get_mesh_for_lods(node_info.lod_level, node_info.neighbor_lods);
        MeshInstance3D *mi = memnew(MeshInstance3D);
        mi->set_mesh(get_mesh_for_lods(node_info.lod_level, node_info.neighbor_lods));
        chunk_node->add_child(mi);
        mi->set_global_position(Vector3(node_info.bounds.position.x, 0.0, node_info.bounds.position.y));
        AABB chunk_aabb;
        chunk_aabb.position.y = -250.0f;
        chunk_aabb.size = Vector3(node_info.bounds.size.x, 750.0, node_info.bounds.size.y);
        mi->set_custom_aabb(chunk_aabb);
        mi->set_instance_shader_parameter(SNAME("sector_size"), node_info.bounds.size.x);
        Ref<RoadChunk> road_chunk = layer->road_layer->get_chunk_at_world_position(node_info.bounds.get_center());
        mi->set_instance_shader_parameter(SNAME("height_texture_start"), road_chunk->get_bounds().position);
        mi->set_instance_shader_parameter(SNAME("height_texture_end"), road_chunk->get_bounds().get_end());
        Ref<InstanceTextureHandle> texture_handle = road_chunk->get_heightmap_texture_handle();
        mi->set_instance_shader_parameter(SNAME("height_normal_texture_idx"), texture_handle->get_idx());
        loaded_grid_nodes.insert(node_infos[node_i].bounds, {
            .mi = mi,
            .lod_level = node_infos[node_i].lod_level,
            .lod_flags = get_tjunction_mesh_flags(node_infos[node_i].lod_level, node_infos[node_i].neighbor_lods)
        });
    }
}

void QuadTreeTerrainChunk::build(tf::Taskflow &p_taskflow) {
    camera_position = layer->get_camera_position();
    p_taskflow.emplace([&]() {
        update_quadtree();
    }).name("Regenerate quadtree");
}

void QuadTreeTerrainChunk::unload() {
    if (chunk_node) {
        chunk_node->queue_free();
        chunk_node = nullptr;
    }
}

void QuadTreeTerrainChunk::on_build_completed() {
}

