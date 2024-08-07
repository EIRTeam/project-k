#ifndef WORLDGEN_COMPOSER_H
#define WORLDGEN_COMPOSER_H

#include "core/object/ref_counted.h"
#include "core/templates/hash_map.h"
#include "scene/resources/texture.h"
#include "modules/noise/fastnoise_lite.h"
#include <iterator>
#include <array>
extern "C" {
    #include "thirdparty/sdnoise.h"
}
#include "voronoise.h"

class WorldGenerator : public RefCounted {
    GDCLASS(WorldGenerator, RefCounted);
    static constexpr int CHUNK_SIZE = 32;

    int64_t seed;

    enum ChunkLODLevel {
        LOD0
    };
    struct ChunkData {
        ChunkLODLevel lod = LOD0;
        Ref<Texture2D> biome_map;
        Ref<Texture2D> normal_map;
        Ref<Texture2D> height_map;
        Vector2i position;
    };

    Ref<FastNoiseLite> biome_map_x_noise;
    Ref<FastNoiseLite> biome_map_y_noise;
    Ref<FastNoiseLite> biome_boundary_distortion_noise;

    Ref<Voronoise> biome_boundaries_noise;
    Ref<Curve> continentalness_curve;
    

    HashMap<Vector2i, ChunkData> loaded_chunks;

    enum Biome {
        GRASS,
        WATER,
        BEACH
    };

    float biome_heights[3] {
        0.75,
        0.0,
        0.5,
    };

    Biome get_biome_for_cell(Vector2i p_cell) const {
        float noise_val = (biome_map_x_noise->get_noise_2d(p_cell.x, p_cell.y) + 1.0f)*0.5f;
        if (noise_val > 0.3f) {
            return GRASS;
        }
        return WATER;
    }

protected:
    static void _bind_methods();
public:
    static Ref<WorldGenerator> create_bind(int64_t p_seed) {
        Ref<WorldGenerator> worldgen;
        worldgen.instantiate(p_seed);
        return worldgen;
    }

    void sample_distorted_biome_boundary(Vector2 p_world_pos, Voronoise::VoronoiSampleResult &r_result) const {
        const float BOUNDARY_BOUNDARY_MULTIPLIER = 0.1f;
        const Vector2 world_pos = p_world_pos + 2.0f * Vector2(biome_boundary_distortion_noise->get_noise_2d(p_world_pos.x, p_world_pos.y), biome_boundary_distortion_noise->get_noise_2d(p_world_pos.y, p_world_pos.x));
        biome_boundaries_noise->sample(world_pos * BOUNDARY_BOUNDARY_MULTIPLIER, r_result);
    }

    bool has_chunk(Vector2i p_chunk_pos) const {
        return loaded_chunks.has(p_chunk_pos);
    }

    static float calculateNormal(float x, float y, Vector2 &p_normal) {
        float dnoise_dx, dnoise_dy;
        float noise = sdnoise2(x, y, &dnoise_dx, &dnoise_dy);
        
        // Calculate the normal
        Vector3 N = Vector3(dnoise_dx, dnoise_dy, 1.0);
        
        // Normalize the normal
        N = N.normalized();
        
        // Remap and store in the normal map (two channels)
        p_normal.x = N.x * 0.5 + 0.5;
        p_normal.y = N.y * 0.5 + 0.5;
        // Convert to OpenGL normals...
        //p_normal.y = 1.0 - p_normal.y;
        return noise;
    }

    float sample_shit(Vector2 p_pos) {
        float p_dx, p_dy;
        float height = sdnoise2(p_pos.x, p_pos.y, &p_dx, &p_dy);
        height = continentalness_curve->sample(height);

        /*Vector3 off = Vector3(1.0, 1.0, 0.0);
        float hL = height(P.xy - off.xz);
        float hR = height(P.xy + off.xz);
        float hD = height(P.xy - off.zy);
        float hU = height(P.xy + off.zy);

        // deduce terrain normal
        N.x = hL - hR;
        N.y = hD - hU;
        N.z = 2.0;
        N = normalize(N);*/

        return height;
    }

    static float noise(const Vector2 &p_pos, Array r_out_derivatives) {
        float dx, dy;
        float height = sdnoise2(p_pos.x, p_pos.y, &dx, &dy);
        r_out_derivatives.push_back(dx);
        r_out_derivatives.push_back(dy);
        return height;
    }



    void generate_chunk(Vector2i p_chunk_pos, Ref<Image> p_image, Ref<Image> p_normal_image) const {
        ERR_FAIL_COND_MSG(loaded_chunks.find(p_chunk_pos), "Chunk already exists");
        int img_size = p_image->get_size().x;

        // Generate biome map
        struct BiomeResult {
            Biome biome;
            Biome second_biome;
            float distances[2];
        };
        LocalVector<BiomeResult> biome_results;
        biome_results.resize(p_image->get_width() * p_image->get_width());


        for(int x = 0; x < p_image->get_width(); x++) {
            float x_world = (x * CHUNK_SIZE) / (float)(img_size-1);
            for(int y = 0; y < p_image->get_width(); y++) {
                int idx = y * p_image->get_width() + x;

                float y_world = (y * CHUNK_SIZE) / (float)(img_size-1);
                Vector2 world_pos = Vector2(x_world, y_world) + (Vector2)p_chunk_pos;
                Voronoise::VoronoiSampleResult sample_result;
                sample_distorted_biome_boundary(world_pos, sample_result);
                biome_results[idx].biome = get_biome_for_cell(sample_result.closest_cell_centers[0]);
                biome_results[idx].second_biome = get_biome_for_cell(sample_result.closest_cell_centers[1]);
                biome_results[idx].distances[0] = sample_result.closest_distances[0];
                biome_results[idx].distances[1] = sample_result.closest_distances[1];
            }
        }

        for (int x = 0; x < img_size; x++) {
            float x_world = (x * CHUNK_SIZE) / (float)(img_size-1);
            for (int y = 0; y < img_size; y++) {
                float pixel_size = (1.0 / (float)img_size);
                float y_world = (y * CHUNK_SIZE) / (float)(img_size-1);
                int idx = y * p_image->get_width() + x;
                Biome biome = biome_results[idx].biome;
                Biome biome_second = biome_results[idx].second_biome;
                if (biome == GRASS && biome != biome_second) {
                    if (biome_results[idx].distances[0] / biome_results[idx].distances[1] > 0.01f) {
                        biome = Biome::BEACH;
                    }
                }

                Vector2 world_pos = Vector2(x_world, y_world) + (Vector2)p_chunk_pos;
                world_pos *= 0.01f;
                float biome_height = 0.0f;
                for(int i = -1; i < 2; i++) {
                    for (int ii = -1; ii < 2; ii++) {
                        int idx_biome = CLAMP(ii + y, 0, p_image->get_width()-1) * p_image->get_width() + CLAMP(i, 0, p_image->get_width()-1);
                        biome_height += biome_heights[biome_results[idx_biome].biome];
                    }
                }
                biome_height /= 9.0f;
                //height *= biome_height;
                Vector2 noise_derivative;
                float height = calculateNormal(world_pos.x, world_pos.y, noise_derivative) * 0.5f + 0.5f;
                float remapped_height = continentalness_curve->sample(height);
                float mul = remapped_height/height; 
                height = remapped_height;

                // Something here
                
                Color col;
                switch(biome) {
                    case Biome::GRASS: {
                        col = Color(0.0, 1.0, 0.0);
                    } break;
                    case Biome::WATER: {
                        col = Color(0.0, 0.0, 1.0);
                    } break;
                    case Biome::BEACH: {
                        col = Color(1.0, 1.0, 0.0);
                    } break;
                }
                col.a = height;
                p_image->set_pixel(x, y, col);
                p_normal_image->set_pixel(x, y, Color(noise_derivative.x, noise_derivative.y, 1.0f, 1.0f));

                // Test numbah 2
                Vector2 dnoise_dxy;
                float height_val = sdnoise2(world_pos.x, world_pos.y, &dnoise_dxy.x, &dnoise_dxy.y);
                height_val = (height_val+1.0f) / 2.0f;
                float pre_height = height_val;
                height_val = continentalness_curve->sample(height_val);
                col.a = height_val;
                p_image->set_pixel(x, y, col);
                float mul2 = height_val/pre_height;
                mul2 *= 1.0f;
                Vector3 norm = Vector3(-dnoise_dxy.x*mul2, 1.0f, -dnoise_dxy.y*mul2);
                norm.normalize();
                norm = (norm + Vector3(1.0f, 1.0f, 1.0f)) / 2.0f;
                p_normal_image->set_pixel(x, y, Color(norm.x, norm.y, norm.z, 1.0f));
            }
        }
    }

    WorldGenerator(int64_t p_seed);
};

class WorldgenComposer : public RefCounted {
    GDCLASS(WorldgenComposer, RefCounted);
public:
};

#endif // WORLDGEN_COMPOSER_H
