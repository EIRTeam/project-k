#include "worldgen_manager.h"
#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/image.h"
#include "core/math/geometry_2d.h"
#include "core/math/transform_3d.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/string/print_string.h"
#include "core/templates/hash_map.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/local_vector.h"
#include "core/templates/pair.h"
#include "core/variant/type_info.h"
#include "core/variant/variant.h"
#include "modules/noise/fastnoise_lite.h"
#include "plane_generate.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/occluder_instance_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/main/node.h"
#include "scene/resources/3d/height_map_shape_3d.h"
#include "scene/resources/curve.h"
#include "scene/resources/image_texture.h"
#include "scene/main/viewport.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/resources/3d/concave_polygon_shape_3d.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer_scene_cull.h"
#include "thirdparty/taskflow/algorithm/for_each.hpp"
#include "worldgen/chunker.h"
#include "worldgen/heightmap.h"
#include "worldgen/quadtree.h"
#include "worldgen/render_layers.h"
#include "worldgen/road_astar.h"
#include "worldgen/thirdparty/taskflow/core/task.hpp"
#include "worldgen/thirdparty/taskflow/core/taskflow.hpp"
#include "worldgen/worldgen_scatter.h"
#include "core/os/time.h"
static bool b = false;

void WorldgenManager::_bind_methods() {
    ADD_SIGNAL(MethodInfo("chunk_loaded", PropertyInfo(Variant::VECTOR2I, "chunk")));
    ADD_SIGNAL(MethodInfo("chunk_doomed", PropertyInfo(Variant::VECTOR2I, "chunk")));
    ADD_SIGNAL(MethodInfo("chunk_resurrected", PropertyInfo(Variant::VECTOR2I, "chunk")));
    ADD_SIGNAL(MethodInfo("chunk_unloaded", PropertyInfo(Variant::VECTOR2I, "chunk")));
    ClassDB::bind_method(D_METHOD("update_chunk_visibility", "camera_pos"), &WorldgenManager::update_chunk_visibility);
    
    ClassDB::bind_method(D_METHOD("set_terrain_material", "terrain_material"), &WorldgenManager::set_terrain_material);
    ClassDB::bind_method(D_METHOD("get_terrain_material"), &WorldgenManager::get_terrain_material);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "terrain_material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_terrain_material", "get_terrain_material");

    ClassDB::bind_method(D_METHOD("set_worldgen_settings", "worldgen_settings"), &WorldgenManager::set_worldgen_settings);
    ClassDB::bind_method(D_METHOD("get_worldgen_settings"), &WorldgenManager::get_worldgen_settings);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "worldgen_settings", PROPERTY_HINT_RESOURCE_TYPE, "WorldgenSettings"), "set_worldgen_settings", "get_worldgen_settings");

    ClassDB::bind_method(D_METHOD("set_chunker_settings", "chunker_settings"), &WorldgenManager::set_chunker_settings);
    ClassDB::bind_method(D_METHOD("get_chunker_settings"), &WorldgenManager::get_chunker_settings);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "chunker_settings", PROPERTY_HINT_RESOURCE_TYPE, "ChunkerQuadTreeSettings", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_chunker_settings", "get_chunker_settings");

    ClassDB::bind_method(D_METHOD("set_scatterer", "scatterer"), &WorldgenManager::set_scatterer);
    ClassDB::bind_method(D_METHOD("get_scatterer"), &WorldgenManager::get_scatterer);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "scatterer", PROPERTY_HINT_RESOURCE_TYPE, "WorldgenScatter", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_scatterer", "get_scatterer");
}

Vector2 WorldgenManager::get_chunk_center(const Vector2i &p_chunk) const {
    int chunk_size = GLOBAL_GET("kgame/chunk_size");
    return Vector2(p_chunk * chunk_size) + Vector2(chunk_size * 0.5f, chunk_size * 0.5f);
}

float WorldgenManager::_sample_height(const Vector2 &p_pos) const {
    return height.get_height(p_pos);
}

void WorldgenManager:: _sample_height_with_derivative(const Vector2 &p_pos, float &r_height, Vector2 &r_derivative, float p_dir_eps) const {
    height.get_height_with_derivative(p_pos, r_height, r_derivative);
}

bool clamp_segment_to_rect2(const Rect2 &p_rect, const Vector2 &p_first, const Vector2 &p_second, Vector2 &r_out_p1, Vector2 &r_out_p2) {
    // just to be sure
    r_out_p1 = p_first;
    r_out_p2 = p_second;

    bool intersected = false;
 
    Vector2 first_out = p_first;
    if (p_rect.intersects_segment(p_first, p_second, &first_out)) {
        r_out_p1 = first_out;
        intersected = true;
    }

    Vector2 second_out = p_second;

    if (p_rect.intersects_segment(p_second, first_out, &second_out)) {
        r_out_p2 = second_out;
        intersected = true;
    }
    return intersected;
}

static BitField<PlaneGenerate::GridTJunctionRemovalFlags> get_tjunction_flags(int p_lod, ChunkerQuadTree::NeighborLODs p_lods) {
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

void WorldgenManager::update_chunk_visibility(const Vector2 &p_camera_position) {
    if (!heightmap_texture.is_valid()) {
        int64_t start = Time::get_singleton()->get_ticks_msec();
        Vector2i heightmap_dimensions = Vector2i(4096, 4096);
        tf::Taskflow tf2;

        Vector<float> data;
        data.resize(heightmap_dimensions.x*heightmap_dimensions.y);
        float *color_data = data.ptrw();

        tf2.for_each_index(0, heightmap_dimensions.x*heightmap_dimensions.y, 1, [this, color_data, heightmap_dimensions](int i) {
            Vector2 pos = Vector2i(i % heightmap_dimensions.x, i / heightmap_dimensions.x);
            float h = _sample_height((pos / Vector2(heightmap_dimensions)) * Vector2(chunker->get_bounds().size));
            color_data[i] = h;
        });
        task_executor.run(tf2).wait();
        Ref<Image> heightmap_img = Image::create_from_data(heightmap_dimensions.x, heightmap_dimensions.y, false, Image::FORMAT_RF, data.to_byte_array());
        heightmap_texture = ImageTexture::create_from_image(heightmap_img);
        print_line(vformat("Heightmap build took %d ms", Time::get_singleton()->get_ticks_msec() - start));
        //terrain_material->set_shader_parameter("heightmap", heightmap_texture);
    }

    tf::Taskflow tf;

    Camera3D *cam = get_viewport()->get_camera_3d();
    if (!cam) {
        return;
    }

    Vector<ChunkerQuadTree::LeafNodeInfo> leaf_nodes;
    int megachunk_radius = GLOBAL_GET("kgame/megachunk_radius");
    int megachunk_size = GLOBAL_GET("kgame/megachunk_size");
    const Vector2i current_megachunk = p_camera_position / megachunk_size;
    for (int megachunk_x = current_megachunk.x-megachunk_radius; megachunk_x <= current_megachunk.x+megachunk_radius; megachunk_x++) {
        for (int megachunk_y = current_megachunk.y-megachunk_radius; megachunk_y <= current_megachunk.y+megachunk_radius; megachunk_y++) {
            Vector2 megachunk_position = Vector2(megachunk_x, megachunk_y) * megachunk_size;
            chunker->clear();
            chunker_settings->set_bounds(Rect2(megachunk_position, Vector2(megachunk_size, megachunk_size)));
            chunker->insert_camera(p_camera_position);
            leaf_nodes.append_array(chunker->get_leaf_node_infos());
        }
    }

    HashMap<Rect2, ChunkerQuadTree::LeafNodeInfo*> leaf_nodes_map;


    Vector<ChunkerQuadTree::LeafNodeInfo*> nodes_to_load;
    Vector<Rect2> nodes_to_unload;

    for (ChunkerQuadTree::LeafNodeInfo &leaf : leaf_nodes) {
        HashMap<Rect2, ChunkInfo>::Iterator it = loaded_nodes.find(leaf.bounds);
        if (it == loaded_nodes.end()) {
            // Node not loaded, queue for loading
            nodes_to_load.push_back(&leaf);
        }
        leaf_nodes_map.insert(leaf.bounds, &leaf);
    }

    for (const KeyValue<Rect2, ChunkInfo> &kv : loaded_nodes) {
        HashMap<Rect2, ChunkerQuadTree::LeafNodeInfo*>::ConstIterator it = leaf_nodes_map.find(kv.key);
        if (it == leaf_nodes_map.end()) {
            // Node needs to be unloaded
            nodes_to_unload.push_back(kv.key);
            continue;
        }
        if (get_tjunction_flags(it->value->lod_level, it->value->neighbor_lods) == kv.value.terrain_mesh_tjunction_flags) {
            // Mesh hasn't changed, no need to do anything
            continue;
        }

        // neighboring lods aren't the same, we need to rebuild it
        nodes_to_unload.push_back(it->key);
        nodes_to_load.push_back(it->value);
    }

    for (const Rect2 &node : nodes_to_unload) {
        unload_node(&loaded_nodes[node]);
        loaded_nodes.erase(node);
    }

    if (nodes_to_load.size() > 0) {
        last_chunk_rebuild_events.push_back(nodes_to_load.size());
        if (last_chunk_rebuild_events.size() > 10) {
            last_chunk_rebuild_events.pop_front();
        }
    }

    LocalVector<ChunkGenerateTaskData> generate_task_datas;
    generate_task_datas.resize(nodes_to_load.size());

    for (int n = 0; n < nodes_to_load.size(); n++) {
        const ChunkerQuadTree::LeafNodeInfo *node = nodes_to_load[n];

        BitField<PlaneGenerate::GridTJunctionRemovalFlags> flags = get_tjunction_flags(node->lod_level, node->neighbor_lods);
        ChunkGenerateTaskData *task_data = &generate_task_datas[n];
        generate_task_datas[n].grid_mesh = node_meshes_arrs[flags];
        generate_task_datas[n].grid_mesh.positions = node_meshes_arrs[flags].positions.duplicate();
        generate_task_datas[n].grid_mesh.normals = Vector<Vector3>();
        generate_task_datas[n].grid_mesh.normals.resize(node_meshes_arrs[flags].positions.size());
        generate_task_datas[n].bounds = nodes_to_load[n]->bounds;
        generate_task_datas[n].lod_level = nodes_to_load[n]->lod_level;
        generate_task_datas[n].uses_pixel_normals = nodes_to_load[n]->lod_level >= worldgen_settings->get_pixel_normals_min_lod();

        Vector3 *pos_arr = generate_task_datas[n].grid_mesh.positions.ptrw();
        Vector3 *normal_arr = generate_task_datas[n].grid_mesh.normals.ptrw();
        tf.for_each_index(0, (int)generate_task_datas[n].grid_mesh.positions.size(), 1, [this, task_data, pos_arr, normal_arr](int i) {
            Vector2 sample_pos = Vector2(task_data->grid_mesh.positions[i].x, task_data->grid_mesh.positions[i].z);

            sample_pos *= task_data->bounds.size;
            sample_pos += task_data->bounds.position;

            float n_height = 0.0f;
            Vector2 deriv;
            _sample_height_with_derivative(sample_pos, n_height, deriv);
            pos_arr[i] *= Vector3(task_data->bounds.size.x, 0.0, task_data->bounds.size.y);
            pos_arr[i].y = n_height;

            Vector3 normal = Vector3(-deriv.x, 1.0f, -deriv.y).normalized();
            normal_arr[i] = normal;
        }).name("Generate vertices and normals");

        if (generate_task_datas[n].uses_pixel_normals) {
            const int HEIGHTMAP_DIMENSIONS = 64;
            tf::Task allocate_normal_map_task = tf.emplace([task_data]() {
                task_data->heightmap = Image::create_empty(HEIGHTMAP_DIMENSIONS, HEIGHTMAP_DIMENSIONS, false, Image::FORMAT_RGBF);
            });
            tf::Task generate_normal_map_task = tf.for_each_index(0, HEIGHTMAP_DIMENSIONS*HEIGHTMAP_DIMENSIONS, 1, [this, task_data](int i) {
                Vector2i pixel_xy = Vector2i(i % HEIGHTMAP_DIMENSIONS, i / HEIGHTMAP_DIMENSIONS);
                Vector2 sample_pos = task_data->bounds.position + (Vector2(i % HEIGHTMAP_DIMENSIONS, i / HEIGHTMAP_DIMENSIONS) / Vector2(HEIGHTMAP_DIMENSIONS-2, HEIGHTMAP_DIMENSIONS-2) * task_data->bounds.size);

                Vector3 normal;
                float o_height;
                height.get_height_with_normal(sample_pos, normal, o_height);
                task_data->heightmap->set_pixelv(pixel_xy, Color(normal.x, normal.y, normal.z, 0.0));
            });

            allocate_normal_map_task.precede(generate_normal_map_task);
        }

        /*tf.emplace([this, task_data]() {
            Vector<WorldgenScatter::ScatterInstance> scatter_result = scatterer->run_scatter(WorldgenScatter::ScatterSettings {
                .lod = task_data->lod_level,
                .region = task_data->bounds,
                .seed = VariantHasher::hash(task_data->bounds)
            }, &height);
            task_data->scatter_results = scatter_result;
        });*/
    }

    task_executor.run(tf).wait();

    for (int i = 0; i < nodes_to_load.size(); i++) {
        const ChunkerQuadTree::LeafNodeInfo *node = nodes_to_load[i];
        Ref<ArrayMesh> mesh;
        mesh.instantiate();
        Array mesh_arr;
        mesh_arr.resize(RS::ARRAY_MAX);
        mesh_arr[RS::ARRAY_VERTEX] = generate_task_datas[i].grid_mesh.positions;
        mesh_arr[RS::ARRAY_INDEX] = generate_task_datas[i].grid_mesh.indices;
        mesh_arr[RS::ARRAY_TEX_UV] = generate_task_datas[i].grid_mesh.uvs;
        mesh_arr[RS::ARRAY_NORMAL] = generate_task_datas[i].grid_mesh.normals;
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mesh_arr);

        Transform3D mesh_trf;
        mesh_trf.origin = Vector3(node->bounds.position.x, 0.0, node->bounds.position.y);
        MeshInstance3D *mi = memnew(MeshInstance3D);
        mi->set_mesh(mesh);
        add_child(mi);
        mi->set_global_transform(mesh_trf);

        mi->set_material_override(terrain_material);
        int heightmap_idx = -1;
        if (generate_task_datas[i].uses_pixel_normals) {
            if (available_heightmap_texture_indices.size() > 0) {
                int texture_index = available_heightmap_texture_indices.front()->get();
                available_heightmap_texture_indices.pop_front();
                mi->set_instance_shader_parameter(SNAME("chunk_heightmap_idx"), texture_index);
                
                heightmap_textures->update_layer(generate_task_datas[i].heightmap, texture_index);
                heightmap_idx = texture_index;
            } else {
                // We have no more space, fall back to using vertex normals
                ERR_PRINT_ONCE("Ran out of space for per-chunk heightmaps!");
                generate_task_datas[i].uses_pixel_normals = false;
            }
        }
        mi->set_instance_shader_parameter(SNAME("use_pixel_normals"), generate_task_datas[i].uses_pixel_normals);

        for (WorldgenScatter::ScatterInstance scatter_result : generate_task_datas[i].scatter_results) {
            Node *scene = scatter_result.scene->get_scene()->instantiate();
            Node3D *node_3d = Object::cast_to<Node3D>(scene);
            if (!node_3d) {
                ERR_PRINT_ONCE("Scatter scene was not derived from Node3D");
                scene->queue_free();
            }

            mi->add_child(node_3d);
            node_3d->set_global_transform(scatter_result.transform);
        }

        /*OccluderInstance3D *occluder = memnew(OccluderInstance3D);
        Ref<ArrayOccluder3D> occluder_mesh;
        occluder_mesh.instantiate();
        occluder_mesh->set_arrays(generate_task_datas[i].grid_mesh.positions, generate_task_datas[i].grid_mesh.indices);
        occluder->set_occluder(occluder_mesh);
        mi->add_child(occluder);*/
        mi->set_layer_mask(1 | RENDER_LAYER_TERRAIN);

        loaded_nodes.insert(node->bounds, {
            .chunk_bounds = node->bounds,
            .lod_level = node->lod_level,
            .terrain_mi = mi,
            .terrain_mesh_tjunction_flags = get_tjunction_flags(node->lod_level, node->neighbor_lods),
            .chunk_heightmap_idx = heightmap_idx
        });
    }
}

Ref<Texture2D> WorldgenManager::get_chunk_height_texture(const Vector2i &p_chunk) const {
    const int CHUNK_TEXTURE_SIZE = 64;
    Ref<Image> image = Image::create_empty(CHUNK_TEXTURE_SIZE, CHUNK_TEXTURE_SIZE, false, Image::FORMAT_RF);

    int chunk_size = GLOBAL_GET("kgame/chunk_size");
    const Vector2 chunk_sample_origin = chunk_size * p_chunk;

    for (int x = 0; x < CHUNK_TEXTURE_SIZE; x++) {
        for (int y = 0; y < CHUNK_TEXTURE_SIZE; y++) {
            Vector2 p = (Vector2(x, y) / Vector2(CHUNK_TEXTURE_SIZE, CHUNK_TEXTURE_SIZE));
            p *= (float)chunk_size;
            const Vector2 chunk_sample = chunk_sample_origin + p;

            image->set_pixel(x, y, Color(_sample_height(chunk_sample), 0.0, 0.0, 1.0f));
        }
    }

    ResourceSaver::save(image, "res://height.res");

    return ImageTexture::create_from_image(image);
}

void WorldgenManager::_notification(int p_what) {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    switch(p_what) {
        case NOTIFICATION_READY: {
            chunker.instantiate();
            chunker->set_settings(chunker_settings);

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
                .element_count = 6,
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
                mesh_arr[RS::ARRAY_NORMAL] = mesh.normals;

                Ref<ArrayMesh> arr_mesh;
                arr_mesh.instantiate();
                arr_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mesh_arr);

                node_meshes.insert(perm, arr_mesh);
                node_meshes_arrs.insert(perm, mesh);
            }

            const int HEIGHTMAP_DIMENSIONS = 64;
            heightmap_textures.instantiate();
            Ref<Image> ref_img = Image::create_empty(HEIGHTMAP_DIMENSIONS, HEIGHTMAP_DIMENSIONS, false, Image::FORMAT_RGBF);
            Vector<Ref<Image>> images;
            images.resize(128);
            images.fill(ref_img);
            heightmap_textures->create_from_images(images);
            for (int i = 0; i < 128; i++) {
                available_heightmap_texture_indices.push_back(i);
            }
            terrain_material->set_shader_parameter("chunk_heightmaps", heightmap_textures);
        } break;
    }
}

void WorldgenManager::clear_chunks() {
}

WorldgenManager::WorldgenManager() {
}

float WorldgenHeight::get_height(const Vector2 &p_position) const {
    //float height = fnl->get_noise_2dv(p_position*frequency) * 0.5f + 0.5f;
    
    float G = exp2(-1);
    float f = settings->get_frequency();
    float a = settings->get_base_ampltiude();
    float t = 0.0;
    for (int i = 0 ; i < settings->get_octaves(); i++) {
        t += a*fnl->get_noise_2dv(f*p_position);
        f *= 2.0;
        a *= G;
    }

    t = (t) * 0.5f + 0.5f; 

    const Ref<Curve> height_curve = settings->get_height_curve();
    const float height_multiplier = settings->get_height_multiplier();


    return height_curve->sample(t) * height_multiplier;
}

void WorldgenHeight::get_height_with_derivative(const Vector2 &p_position, float &r_height, Vector2 &r_derivative, float p_dir_eps) const {
    float sample_height = get_height(p_position);
    float h2 = get_height(p_position + Vector2(p_dir_eps, 0.0f));
    float h3 = get_height(p_position + Vector2(0.0f, p_dir_eps));

    r_height = sample_height;

    float slope_x = h2 - sample_height;
    float slope_z = h3 - sample_height;
    r_derivative = Vector2(slope_x, slope_z) / p_dir_eps;
}

void WorldgenHeight::get_height_with_normal(const Vector2 &p_position, Vector3 &r_normal, float &r_height) const {
    Vector2 deriv;
    float height;
    get_height_with_derivative(p_position, height, deriv);

    Vector3 normal = Vector3(-deriv.x, 1.0f, -deriv.y).normalized();
    r_normal = normal;
    r_height = height;
}

WorldgenHeight::WorldgenHeight() {
    fnl.instantiate();
    fnl->set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);
    fnl->set_fractal_type(FastNoiseLite::FRACTAL_NONE);
}

int WorldgenHeight::get_seed() const {
    return seed;
}

void WorldgenHeight::set_seed(int p_seed) {
    fnl->set_seed(p_seed);
}

void WorldgenSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_height_multiplier"), &WorldgenSettings::get_height_multiplier);
    ClassDB::bind_method(D_METHOD("set_height_multiplier", "height_multiplier"), &WorldgenSettings::set_height_multiplier);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_multiplier"), "set_height_multiplier", "get_height_multiplier");

    ClassDB::bind_method(D_METHOD("get_height_curve"), &WorldgenSettings::get_height_curve);
    ClassDB::bind_method(D_METHOD("set_height_curve", "height_curve"), &WorldgenSettings::set_height_curve);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "height_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_height_curve", "get_height_curve");

    ClassDB::bind_method(D_METHOD("get_frequency"), &WorldgenSettings::get_frequency);
    ClassDB::bind_method(D_METHOD("set_frequency", "frequency"), &WorldgenSettings::set_frequency);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frequency"), "set_frequency", "get_frequency");

    ClassDB::bind_method(D_METHOD("get_octaves"), &WorldgenSettings::get_octaves);
    ClassDB::bind_method(D_METHOD("set_octaves", "octaves"), &WorldgenSettings::set_octaves);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "octaves", PROPERTY_HINT_RANGE, "1,10"), "set_octaves", "get_octaves");

    ClassDB::bind_method(D_METHOD("get_base_ampltidue"), &WorldgenSettings::get_base_ampltiude);
    ClassDB::bind_method(D_METHOD("set_base_ampltidue", "base_amplitude"), &WorldgenSettings::set_base_ampltiude);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_amplitude", PROPERTY_HINT_RANGE, "0.1,10,0.01"), "set_base_ampltidue", "get_base_ampltidue");

    ClassDB::bind_method(D_METHOD("get_grass_mesh"), &WorldgenSettings::get_grass_mesh);
    ClassDB::bind_method(D_METHOD("set_grass_mesh", "grass_mesh"), &WorldgenSettings::set_grass_mesh);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "grass_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_grass_mesh", "get_grass_mesh");

    ClassDB::bind_method(D_METHOD("get_road_profile"), &WorldgenSettings::get_road_profile);
    ClassDB::bind_method(D_METHOD("set_road_profile", "road_profile"), &WorldgenSettings::set_road_profile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "road_profile", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), "set_road_profile", "get_road_profile");

    ClassDB::bind_method(D_METHOD("get_road_width"), &WorldgenSettings::get_road_width);
    ClassDB::bind_method(D_METHOD("set_road_width", "road_width"), &WorldgenSettings::set_road_width);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "road_width"), "set_road_width", "get_road_width");
}

WorldgenSettings::WorldgenSettings() {}
