#ifndef QUADTREE_LAYER_H
#define QUADTREE_LAYER_H

#include "core/math/rect2.h"
#include "layer_manager.h"
#include "../thirdparty/taskflow/core/taskflow.hpp"
#include "../plane_generate.h"
#include "scene/3d/mesh_instance_3d.h"
#include "worldgen/chunker.h"
class QuadTreeTerrainChunk;

class QuadTreeTerrainLayer : public ChunkerLayer {
    HashMap<BitField<PlaneGenerate::GridTJunctionRemovalFlags>, Ref<Mesh>> grid_meshes;
    Ref<ChunkerQuadTreeSettings> quad_tree_settings;
    Vector2 camera_position;
public:
    QuadTreeTerrainLayer() {
        quad_tree_settings.instantiate();
        const float chunk_size = get_chunk_size();
        quad_tree_settings->set_bounds(Rect2(Vector2(), Vector2(chunk_size, chunk_size)));

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
            grid_meshes.insert(perm, gpu_mesh);
        }
    }
    virtual float get_chunk_size() const override {
        return 4096.0f;
    }
    Vector2 get_camera_position() const { return camera_position; }
    void set_camera_position(const Vector2 &camera_position_) { camera_position = camera_position_; }
    friend class QuadTreeTerrainChunk;
};

class QuadTreeTerrainChunk : public ChunkerChunk {
    QuadTreeTerrainLayer *layer = nullptr;
    Ref<ChunkerQuadTree> quad_tree;
    Vector2 camera_position;

    struct GridNode {
        MeshInstance3D *mi = nullptr;
        int lod_level;
        BitField<PlaneGenerate::GridTJunctionRemovalFlags> lod_flags;
    };

    HashMap<Rect2, GridNode> loaded_grid_nodes;

    QuadTreeTerrainChunk(QuadTreeTerrainLayer *p_layer) {
        layer = p_layer;
        quad_tree.instantiate(layer->quad_tree_settings);
    }
    virtual void build(tf::Taskflow &p_taskflow) override {
        camera_position = layer->get_camera_position();
        p_taskflow.emplace([&]() {
            quad_tree->clear();
            quad_tree->insert_camera(camera_position);
        }).name("Regenerate quadtree");
    }

    static BitField<PlaneGenerate::GridTJunctionRemovalFlags> get_tjunction_mesh_flags(int p_lod, ChunkerQuadTree::NeighborLODs p_lods) {
        BitField<PlaneGenerate::GridTJunctionRemovalFlags> flags;

        if (p_lods[ChunkerQuadTree::DIR_N] != -1 && p_lods[ChunkerQuadTree::DIR_N] < p_lod) {
            flags.set_flag(PlaneGenerate::GridTJunctionRemovalFlags::UP);
        }
        if (p_lods[ChunkerQuadTree::DIR_S] != -1 && p_lods[ChunkerQuadTree::DIR_S] < p_lod) {
            flags.set_flag(PlaneGenerate::GridTJunctionRemovalFlags::DOWN);
        }
        if (p_lods[ChunkerQuadTree::DIR_E] != -1 && p_lods[ChunkerQuadTree::DIR_E] < p_lod) {
            flags.set_flag(PlaneGenerate::GridTJunctionRemovalFlags::RIGHT);
        }
        if (p_lods[ChunkerQuadTree::DIR_W] != -1 && p_lods[ChunkerQuadTree::DIR_W] < p_lod) {
            flags.set_flag(PlaneGenerate::GridTJunctionRemovalFlags::LEFT);
        }
        return flags;
    }

    Ref<Mesh> get_mesh_for_lods(int p_lod, ChunkerQuadTree::NeighborLODs p_neighbor_lods) {
        return layer->grid_meshes[get_tjunction_mesh_flags(p_lod, p_neighbor_lods)];
    }

    virtual void on_build_completed() override {
        // Find which grid nodes we have, and see if we need to swap the mesh
        Vector<ChunkerQuadTree::LeafNodeInfo> node_infos = quad_tree->get_leaf_node_infos();
        HashSet<Rect2> node_bounds;
        Vector<int> nodes_to_create;
        for (int i = 0; node_infos.size(); i++) {
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
    }
};

#endif // QUADTREE_LAYER_H
