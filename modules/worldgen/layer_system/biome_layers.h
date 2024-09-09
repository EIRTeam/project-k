#ifndef BIOME_LAYERS_H
#define BIOME_LAYERS_H

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/math/geometry_2d.h"
#include "core/math/random_number_generator.h"
#include "core/math/rect2.h"
#include "core/object/ref_counted.h"
#include "core/string/print_string.h"
#include "core/templates/hashfuncs.h"
#include "core/variant/typed_array.h"
#include "core/variant/variant.h"
#include "layer_manager.h"
#include "modules/noise/fastnoise_lite.h"
#include "worldgen/thirdparty/taskflow/core/taskflow.hpp"
#include "worldgen/voronoi.h"

class BiomeSettings : public Resource {
    GDCLASS(BiomeSettings, Resource);
    float height_multiplier = 1.0f;
    float reference_height = 1.0f;
    Ref<FastNoiseLite> noise;
    Rect2 selector_rect;
protected:
    static void _bind_methods();
public:
    float get_height_multiplier() const { return height_multiplier; }
    void set_height_multiplier(float p_height_multiplier) { height_multiplier = p_height_multiplier; }

    float get_reference_height() const { return reference_height; }
    void set_reference_height(float p_reference_height) { reference_height = p_reference_height; }

    Ref<FastNoiseLite> get_noise() const { return noise; }
    void set_noise(Ref<FastNoiseLite> p_noise) { noise = p_noise; }

    Rect2 get_selector_rect() const { return selector_rect; }
    void set_selector_rect(const Rect2 &p_selector_rect) { selector_rect = p_selector_rect; }
};

class BiomeGeneratorSettings : public Resource {
    GDCLASS(BiomeGeneratorSettings, Resource);
    Ref<Noise> x_noise;
    Ref<Noise> y_noise;

    Vector<Ref<BiomeSettings>> biomes;

protected:
    static void _bind_methods();

public:
    Ref<Noise> get_x_noise() const { return x_noise; }
    void set_x_noise(Ref<Noise> p_x_noise) { x_noise = p_x_noise; }

    Ref<Noise> get_y_noise() const { return y_noise; }
    void set_y_noise(Ref<Noise> p_y_noise) { y_noise = p_y_noise; }
    TypedArray<BiomeSettings> get_biomes_bind() const {
        TypedArray<BiomeSettings> out;
        for (Ref<BiomeSettings> biome : biomes) {
            out.push_back(biome);
        }
        return out;
    }
    void set_biomes_bind(TypedArray<BiomeSettings> p_biomes) {
        biomes.clear();
        for (Ref<BiomeSettings> biome : p_biomes) {
            biomes.push_back(biome);
        }
    }

    Vector<Ref<BiomeSettings>> get_biomes() const {
        return biomes;
    }
};

class BiomeVoronoiPointsChunk : public ChunkerChunk {
    int point_dimensions_per_chunk = 2;
    LocalVector<Vector2> grid_points;
    virtual void build(tf::Taskflow &p_taskflow) override {
        p_taskflow.emplace([&]() {
            const float grid_size = bounds.size.x / (float)point_dimensions_per_chunk;
            Ref<RandomNumberGenerator> rng;
            rng.instantiate();
            rng->set_seed(HashMapHasherDefault::hash(bounds.position));
            for (int x = 0; x < point_dimensions_per_chunk; x++) {
                for (int y = 0; y < point_dimensions_per_chunk; y++) {
                    Vector2 grid_center = Vector2(x * grid_size, y * grid_size);
                    grid_center += Vector2(grid_size, grid_size) * 0.5f;
                    Vector2 grid_offset;
                    grid_offset.x += rng->randf_range(-0.5, 0.5);
                    grid_offset.y += rng->randf_range(-0.5, 0.5);
                    grid_offset *= grid_size;

                    grid_points.push_back(bounds.position + grid_center + grid_offset);
                }
            }
        }).name("Build voronoi points chunk");
    }
public:
    LocalVector<Vector2> get_grid_points() const {
        return grid_points;
    }
};

class BiomeVoronoiPointsLayer : public ChunkerLayer {
public:
    PackedVector2Array get_points_in_rect(Rect2 p_rect) const {
        PackedVector2Array out;
        for (const KeyValue<Vector2i, Ref<ChunkerChunk>> &kv : loaded_chunks) {
            Ref<BiomeVoronoiPointsChunk> chunk = kv.value;
            if (chunk->get_bounds().intersects(p_rect, true)) {
                for (const Vector2 &pos : chunk->get_grid_points()) {
                    out.push_back(pos);
                }
            }
        }
        return out;
    }
    virtual float get_chunk_size() const override {
        return 2048.0f;
    }

    virtual Ref<ChunkerChunk> create_chunk(int p_lod_level) const override {
        Ref<BiomeVoronoiPointsChunk> chunk;
        chunk.instantiate();
        return chunk;
    }
};

class BiomeVoronoiTriangulationChunk;

class BiomeVoronoiTriangulationLayer : public ChunkerLayer {
    Ref<BiomeVoronoiPointsLayer> points_layer;
public:
    BiomeVoronoiTriangulationLayer(Ref<BiomeVoronoiPointsLayer> p_points_layer) {
        points_layer = p_points_layer;
    }
    virtual float get_chunk_size() const override {
        return 2048.0f;
    }
    virtual float get_chunk_padding() const override {
        return 1024.0f;
    }
    virtual Ref<ChunkerChunk> create_chunk(int p_lod_level) const override;
};

class BiomeVoronoiTriangulationChunk : public ChunkerChunk {
    BiomeVoronoiTriangulationLayer* layer;
    Ref<BiomeVoronoiPointsLayer> point_layer;
    Ref<BiomeGeneratorSettings> biome_settings;
    Ref<VoronoiGraph> graph;
    struct VoronoiTriangle {
        Rect2 bounds;
        int sites[3];
    };
    HashMap<Vector3i, VoronoiTriangle> triangles;
    
    struct SiteBiomeInformation {
        Ref<BiomeSettings> biome;
    };
    LocalVector<SiteBiomeInformation> site_biome_infos;
public:
    BiomeVoronoiTriangulationChunk() {
        biome_settings = ResourceLoader::load(GLOBAL_GET("kgame/terrain/biome_settings"));
    }
    virtual void build(tf::Taskflow &p_taskflow) {
        p_taskflow.emplace([&]() {
            Rect2 rect = bounds.grow(layer->get_chunk_padding()*2.0f);
            graph = VoronoiGraph::create(point_layer->get_points_in_rect(rect));
            for (int i = 0; i < graph->get_site_count(); i++) {
                Vector<PackedInt32Array> triangles_idx = graph->get_site_triangles(i);
                for (int t = 0; t < triangles_idx.size(); t++) {
                    std::array<int, 3> triangle_sites = {
                        triangles_idx[t][0],
                        triangles_idx[t][1],
                        i
                    };
                    std::sort(triangle_sites.begin(), triangle_sites.end());

                    if (triangles.has(Vector3i(triangle_sites[0], triangle_sites[1], triangle_sites[2]))) {
                        continue;
                    }

                    Rect2 triangle_rect = Rect2(graph->get_site_position(triangle_sites[0]), Vector2());
                    triangle_rect.expand_to(graph->get_site_position(triangle_sites[1]));
                    triangle_rect.expand_to(graph->get_site_position(triangle_sites[2]));

                    triangles.insert(Vector3i(triangle_sites[0], triangle_sites[1], triangle_sites[2]), {
                        .bounds = triangle_rect,
                        .sites = {
                            triangle_sites[0],
                            triangle_sites[1],
                            triangle_sites[2]
                        },
                    });
                }
            }

            site_biome_infos.resize(graph->get_site_count());
            SiteBiomeInformation *site_biome_infos_ptrw = site_biome_infos.ptr();
            Ref<FastNoiseLite> x_noise = biome_settings->get_x_noise();
            Ref<FastNoiseLite> y_noise = biome_settings->get_y_noise();
            Vector<Ref<BiomeSettings>> biomes = biome_settings->get_biomes();
            for (size_t i = 0; i < site_biome_infos.size(); i++) {
                const Vector2 site_position = graph->get_site_position(i);
                const float biome_selection_point_x = x_noise->get_noise_2dv(site_position) * 0.5f + 0.5f;
                const float biome_selection_point_y = y_noise->get_noise_2dv(site_position) * 0.5f + 0.5f;

                Ref<BiomeSettings> biome;

                for (Ref<BiomeSettings> b : biomes) {
                    if (b->get_selector_rect().has_point(Vector2(biome_selection_point_x, biome_selection_point_y))) {
                        biome = b;
                        break;
                    }
                }

                DEV_ASSERT(biome.is_valid());

                site_biome_infos_ptrw[i] = {
                    .biome = biome
                };
            }
        }).name("Build voronoi diagram");
    }
    struct BiomeInterpInfo {
        Ref<BiomeSettings> biome;
        float weight = 0.0f;
    };

    void barycentric(Vector2 p_p, Vector2 p_a, Vector2 p_b, Vector2 p_c, float &r_u, float &r_v, float &r_w) {
        Vector2 v0 = p_b - p_a;
        Vector2 v1 = p_c - p_a;
        Vector2 v2 = p_p - p_a;
        float den = v0.x * v1.y - v1.x * v0.y;
        r_v = (v2.x * v1.y - v1.x * v2.y) / den;
        r_w = (v0.x * v2.y - v2.x * v0.y) / den;
        r_u = 1.0f - r_v - r_w;
    }


    bool get_biomes_at_point(Vector2 p_point, BiomeInterpInfo r_intep_info[3]) {
        for (const KeyValue<Vector3i, VoronoiTriangle> &kv : triangles) {
            /*if (!kv.value.bounds.has_point(p_point)) {
                continue;
            }*/

            Vector2 site_positions[3] = {
                graph->get_site_position(kv.value.sites[0]),
                graph->get_site_position(kv.value.sites[1]),
                graph->get_site_position(kv.value.sites[2])
            };

            if (!Geometry2D::is_point_in_triangle(p_point, site_positions[0], site_positions[1], site_positions[2])) {
                continue;
            }
            float weights[3];
            barycentric(p_point, site_positions[0], site_positions[1], site_positions[2], weights[0], weights[1], weights[2]);

            for (int i = 0; i < 3; i++) {
                r_intep_info[i] = {
                    .biome = site_biome_infos[kv.value.sites[i]].biome,
                    .weight = weights[i]
                };
            }

            return true;
        }

        return false;
    }

    friend class BiomeVoronoiTriangulationLayer;
};

#endif // BIOME_LAYERS_H
