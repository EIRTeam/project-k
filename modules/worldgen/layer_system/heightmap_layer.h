#ifndef TERRAIN_LAYERS_H
#define TERRAIN_LAYERS_H

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/image.h"
#include "layer_manager.h"
#include "worldgen/chunker.h"
#include "worldgen/bilinear_array.h"
#include "../thirdparty/taskflow/core/task.hpp"
#include "../thirdparty/taskflow/core/taskflow.hpp"
#include "worldgen/instance_texture_queue.h"
#include "worldgen/layer_system/biome_layers.h"
#include "../thirdparty/taskflow/algorithm/for_each.hpp"
#include "worldgen/worldgen_height.h"

class HeightmapLayer;

class HeightmapChunk : public ChunkerChunk {
    GDCLASS(HeightmapChunk, ChunkerChunk);
    int heightmap_dimensions;
    Ref<WorldgenHeight> height_source;
    Ref<WorldBoundBilinearArray> heightmap_array;
    Ref<BiomeVoronoiTriangulationLayer> biomes_layer;
public:
    HeightmapChunk(Ref<BiomeVoronoiTriangulationLayer> p_biomes_layer) {
        biomes_layer = p_biomes_layer;
        heightmap_dimensions = GLOBAL_GET("kgame/terrain/normal_height_texture_size");
        height_source.instantiate();
        height_source->set_settings(ResourceLoader::load(GLOBAL_GET("kgame/terrain/height_settings")));
    }
    virtual void build(tf::Taskflow &p_taskflow) override {
        tf::Task allocate_task = p_taskflow.emplace([&]() {
            heightmap_array = WorldBoundBilinearArray::create(heightmap_dimensions, bounds);
        }).name("Allocate heightmap array");
        tf::Task generate_task = p_taskflow.for_each_index(0, heightmap_dimensions*heightmap_dimensions, 1, [&](int i) {
            Vector2 progress = Vector2(i % heightmap_dimensions, Math::floor((float)i / heightmap_dimensions-1)) / Vector2(heightmap_dimensions-1, heightmap_dimensions-1);
            Vector2i pixel_xy = Vector2i(i % heightmap_dimensions, i / heightmap_dimensions);
            Vector2 sample_pos = bounds.position + ( progress * bounds.size);
            
            Ref<BiomeVoronoiTriangulationChunk> voronoi_chunk = biomes_layer->get_chunk_at_world_position(sample_pos);
            BiomeVoronoiTriangulationChunk::BiomeInterpInfo biome_infos[3];
            bool found_biomes = voronoi_chunk->get_biomes_at_point(sample_pos, biome_infos);
            float height = 0.0f;
            if (found_biomes) {
                const float weights_pow2[3] = {
                    biome_infos[0].weight * biome_infos[0].weight,
                    biome_infos[1].weight * biome_infos[1].weight,
                    biome_infos[2].weight * biome_infos[2].weight
                };
                const float total_weights = weights_pow2[0] + weights_pow2[1] + weights_pow2[2];
                for (int j = 0; j < 3; j++) {
                    Ref<BiomeSettings> biome = biome_infos[j].biome;
                    float biome_height = biome->get_reference_height() + (biome->get_noise()->get_noise_2dv(sample_pos) * 0.5 + 0.5) * biome->get_height_multiplier();
                    height += (weights_pow2[j]/total_weights) * biome_height;
                }
            }
            DEV_ASSERT(found_biomes);

            heightmap_array->set_pixel(pixel_xy, height);
            
        }).name("Generate heightmap");
        allocate_task.precede(generate_task);
    }
    friend class HeightmapLayer;
};

class HeightmapLayer : public ChunkerLayer {
    GDCLASS(HeightmapLayer, ChunkerLayer);
    Ref<BiomeVoronoiTriangulationLayer> biomes_layer;
public:
    HeightmapLayer(Ref<BiomeVoronoiTriangulationLayer> p_biomes_layer);
    virtual float get_chunk_size() const override;
    virtual float get_chunk_padding() const override;
    virtual Ref<ChunkerChunk> create_chunk(int p_lod_level) const override;

    Ref<HeightmapChunk> get_chunk_at_world_position(Vector2 p_world_position) const;
    float sample_height_at_position(Vector2 p_world_position) const;
    void sample_height_with_derivative_at_position(Vector2 p_world_position, float p_dir_eps, float &r_height, Vector2 &r_derivative) const;
};

#endif // TERRAIN_LAYERS_H
