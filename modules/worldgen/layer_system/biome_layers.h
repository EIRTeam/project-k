#ifndef BIOME_LAYERS_H
#define BIOME_LAYERS_H

#include "core/math/random_number_generator.h"
#include "core/object/ref_counted.h"
#include "core/templates/hashfuncs.h"
#include "layer_manager.h"

class BiomeVoronoiPointsChunk : public ChunkerChunk {
    int point_dimensions_per_chunk = 2;
    LocalVector<Vector2> grid_points;
    virtual void build(tf::Taskflow &p_taskflow) {
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
                    grid_offset *= bounds.size.x;
                    
                    grid_points.push_back(grid_center + grid_offset);
                }
            }
        });
    }
};

class BiomeVoronoiPointsLayer : public ChunkerLayer {
    virtual float get_chunk_size() const override {
        return 2048.0f;
    }
};

class BiomeVoronoiTriangulationLayer : public ChunkerLayer {
    virtual float get_chunk_size() const override {
        return 2048.0f;
    }
    virtual float get_chunk_padding() const override {
        return 1024.0f;
    }
};

#endif // BIOME_LAYERS_H
