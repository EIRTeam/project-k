#ifndef WORLDGEN_MANAGER_H
#define WORLDGEN_MANAGER_H

#include "core/math/vector2.h"
#include "core/object/callable_method_pointer.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/os/mutex.h"
#include "core/variant/type_info.h"
#include "core/variant/variant.h"
#include "core/variant/variant_utility.h"
#include "scene/resources/curve.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/texture.h"
#include "thirdparty/taskflow/taskflow.hpp"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "servers/rendering/renderer_scene_cull.h"
#include "worldgen/chunker.h"
#include "worldgen/heightmap.h"
#include "worldgen/plane_generate.h"
#include "worldgen/road_astar.h"
#include "worldgen/thirdparty/taskflow/core/taskflow.hpp"
#include "chunker.h"
#include "worldgen/worldgen_scatter.h"

class FastNoiseLite;
class StaticBody3D;

class WorldgenDebug;

class RoadNetworkGenerator : public RefCounted {
    GDCLASS(RoadNetworkGenerator, RefCounted);
    struct RoadNetworkSettings {
        int map_size_meters = 2048.0f;
        float road_network_margin = 128.0f;
    };
    RoadNetworkSettings settings;

    enum RoadNodeType {
        FARM,
        CROSSROAD,
        TOWN
    };

    struct RoadNode {
        Vector2 world_position;
        Vector<int> connections = {};
    };


    LocalVector<RoadNode> nodes;
public:
    void generate() {
        Vector<int> unreached_nodes;
        Vector<int> reached_nodes;
        for (int i = 0; i < 5; i++) {
            float x = VariantUtilityFunctions::randf_range(settings.road_network_margin, settings.map_size_meters-settings.road_network_margin);
            float y = VariantUtilityFunctions::randf_range(settings.road_network_margin, settings.map_size_meters-settings.road_network_margin);
            Vector2 node_pos = Vector2(x, y);
            nodes.push_back({
                .world_position = node_pos
            });
            unreached_nodes.push_back(i);
        }

        reached_nodes.push_back(unreached_nodes[unreached_nodes.size()-1]);
        unreached_nodes.remove_at(unreached_nodes.size()-1);
        while (unreached_nodes.size() > 0) {
            int index_reached = 0;
            int index_unreached = 0;
            float record = 100000000.0f;
            for (int r = 0; r < reached_nodes.size(); r++) {
                for (int u = 0; u < unreached_nodes.size(); u++) {
                    float dist = nodes[reached_nodes[r]].world_position.distance_to(nodes[unreached_nodes[u]].world_position);
                    if (dist < record) {
                        index_reached = r;
                        index_unreached = u;
                        record = dist;
                    }
                }
            }

            int ur = unreached_nodes[index_unreached];
            unreached_nodes.remove_at(index_unreached);
            int rn = reached_nodes[index_reached];
            reached_nodes.push_back(ur);

            nodes[ur].connections.push_back(rn);
        }
    }
    
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("generate"), &RoadNetworkGenerator::generate);
        ClassDB::bind_method(D_METHOD("get_road_network_connection_positions"), &RoadNetworkGenerator::get_road_network_connection_positions);
        ClassDB::bind_method(D_METHOD("intersect_roads_with_chunk", "chunk_rect"), &RoadNetworkGenerator::intersect_roads_with_chunk);
    }

    PackedVector2Array get_road_network_connection_positions() const {
        PackedVector2Array out_arr;
        for (uint32_t i = 0; i < nodes.size(); i++) {
            for (int c = 0; c < nodes[i].connections.size(); c++) {
                out_arr.push_back(nodes[i].world_position);
                out_arr.push_back(nodes[nodes[i].connections[c]].world_position);
            }
        }
        return out_arr;
    }

    PackedVector2Array intersect_roads_with_chunk(Rect2 p_chunk_rect) const {
        PackedVector2Array intersecting_roads;

        for(uint32_t i = 0; i < nodes.size(); i++) {
            Vector2 segment_first = nodes[i].world_position;
            for (int conn : nodes[i].connections) {
                Vector2 segment_second = nodes[conn].world_position;
                Vector2 entry_point_1;
                if (p_chunk_rect.intersects_segment(segment_first, segment_second, &entry_point_1)) {
                    Vector2 entry_point_2;
                    p_chunk_rect.intersects_segment(segment_second, segment_first, &entry_point_2);
                    intersecting_roads.push_back(entry_point_1);
                    intersecting_roads.push_back(entry_point_2);
                }
            }
        }

        return intersecting_roads;
    }

public:
    RoadNetworkGenerator() {

    }
};

class WorldgenSettings : public Resource {
    GDCLASS(WorldgenSettings, Resource);
    float height_multiplier = 1.0f;
    float frequency = 0.15f;
    int octaves = 8;
    float base_ampltiude = 1.0f;
    Ref<Curve> height_curve;
    Ref<Curve> road_profile;
    float road_width = 3.0f;
    Ref<Mesh> grass_mesh;
    int pixel_normals_min_lod = 5;
protected:
    static void _bind_methods();
public:
    float get_height_multiplier() const { return height_multiplier; }
    void set_height_multiplier(float p_height_multiplier) { height_multiplier = p_height_multiplier; }

    float get_frequency() const { return frequency; }
    void set_frequency(float p_frequency) { frequency = p_frequency; }

    Ref<Curve> get_height_curve() const { return height_curve; }
    void set_height_curve(const Ref<Curve> &p_height_curve) { height_curve = p_height_curve; }

    int get_octaves() const { return octaves; }
    void set_octaves(int p_octaves) { octaves = p_octaves; }

    float get_base_ampltiude() const { return base_ampltiude; }
    void set_base_ampltiude(float p_base_ampltiude) { base_ampltiude = p_base_ampltiude; }

    Ref<Mesh> get_grass_mesh() const { return grass_mesh; }
    void set_grass_mesh(Ref<Mesh> p_grass_mesh) { return grass_mesh = p_grass_mesh; }

    float get_road_width() const { return road_width; }
    void set_road_width(float p_road_width) { return grass_mesh = p_road_width; }

    Ref<Curve> get_road_profile() const {
        return road_profile;
    }

    void set_road_profile(Ref<Curve> p_road_profile) {
        road_profile = p_road_profile;
    }

    int get_pixel_normals_min_lod() const { return pixel_normals_min_lod; }
    void set_pixel_normals_min_lod(int p_pixel_normals_min_lod) { pixel_normals_min_lod = p_pixel_normals_min_lod; }

    WorldgenSettings();
};

class WorldgenHeight {
    Ref<FastNoiseLite> fnl;
    Ref<WorldgenSettings> settings;
    int seed;
public:
    float get_height(const Vector2 &p_position) const;
    void get_height_with_derivative(const Vector2 &p_position, float &r_height, Vector2 &r_derivative, float p_dir_eps = 0.01f) const;
    void get_height_with_normal(const Vector2 &p_position, Vector3 &r_normal, float &r_height) const;

    int get_seed() const;
    void set_seed(int p_seed);

    Ref<WorldgenSettings> get_settings() const { return settings; }
    void set_settings(const Ref<WorldgenSettings> &p_settings) { settings = p_settings; }

    WorldgenHeight();
};

class WorldgenManager : public Node3D {
    GDCLASS(WorldgenManager, Node3D);

    tf::Taskflow chunk_generation_flow;
    tf::Executor task_executor;

    WorldgenHeight height;

    RoadAStar::RoadAStarSettings base_astar_settings;

    struct ChunkInfo {
        Rect2 chunk_bounds;
        int lod_level = 0;
        MeshInstance3D *terrain_mi = nullptr;
        BitField<PlaneGenerate::GridTJunctionRemovalFlags> terrain_mesh_tjunction_flags;
        RendererSceneCull::InstanceBounds frustum_bounds;
        int chunk_heightmap_idx = -1;
    };

    HashMap<BitField<PlaneGenerate::GridTJunctionRemovalFlags>, Ref<ArrayMesh>> node_meshes;
    HashMap<BitField<PlaneGenerate::GridTJunctionRemovalFlags>, PlaneGenerate::GridMesh> node_meshes_arrs;


    HashMap<Rect2, ChunkInfo> loaded_nodes;

    Ref<ShaderMaterial> terrain_material;
    Ref<WorldgenSettings> worldgen_settings;
    Ref<ChunkerQuadTree> chunker;
    List<int> last_chunk_rebuild_events;

    struct ChunkGenerateTaskData {
        Rect2 bounds;
        int lod_level;
        bool uses_pixel_normals;
        PlaneGenerate::GridMesh grid_mesh;
        Vector<WorldgenScatter::ScatterInstance> scatter_results;
        Ref<Image> heightmap;
    };

    Ref<Texture2D> heightmap_texture;
    Ref<Texture2DArray> heightmap_textures;
    List<int> available_heightmap_texture_indices;

    Ref<ChunkerQuadTreeSettings> chunker_settings;
    Ref<WorldgenScatter> scatterer;

    float _sample_height(const Vector2 &p_pos) const;
    void _sample_height_with_derivative(const Vector2 &p_pos, float &r_height, Vector2 &r_derivative, float p_dir_eps = 0.01) const;
protected:
    static void _bind_methods();
public:
    Vector2 get_chunk_origin(const Vector2i &p_chunk) const;
    Vector2 get_chunk_center(const Vector2i &p_chunk) const;
    
    void update_chunk_visibility(const Vector2 &camera_position);
    Ref<Texture2D> get_chunk_height_texture(const Vector2i &p_chunk) const;
    void _notification(int p_what);

    Ref<ShaderMaterial> get_terrain_material() const { return terrain_material; }
    void set_terrain_material(const Ref<ShaderMaterial> &p_terrain_material) { terrain_material = p_terrain_material; }

    Ref<WorldgenSettings> get_worldgen_settings() const { return worldgen_settings; }
    void set_worldgen_settings(const Ref<WorldgenSettings> &p_worldgen_settings) {
        worldgen_settings = p_worldgen_settings;
        height.set_settings(p_worldgen_settings);
    }

    void unload_node(ChunkInfo *p_info) {
        p_info->terrain_mi->queue_free();
        if (p_info->chunk_heightmap_idx != -1) {
            available_heightmap_texture_indices.push_back(p_info->chunk_heightmap_idx);
        }
    }

    void clear_chunks();

    WorldgenManager();
    Ref<ChunkerQuadTreeSettings> get_chunker_settings() const { return chunker_settings; }
    void set_chunker_settings(const Ref<ChunkerQuadTreeSettings> &p_chunker_settings) {
        chunker_settings = p_chunker_settings;
        // Chunker might not be initialized if we are in the editor
        if (chunker.is_valid()) {
            chunker->set_settings(p_chunker_settings);
        }
    }
    Ref<WorldgenScatter> get_scatterer() const { return scatterer; }
    void set_scatterer(const Ref<WorldgenScatter> &p_scatterer) { scatterer = p_scatterer; }
    friend class WorldgenDebug;
};

#endif // WORLDGEN_MANAGER_H
