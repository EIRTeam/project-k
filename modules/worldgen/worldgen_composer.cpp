#include "worldgen_composer.h"

void Voronoise::_bind_methods() {
    ClassDB::bind_static_method("Voronoise", D_METHOD("create", "map_seed"), &Voronoise::create_bind);
    ClassDB::bind_method(D_METHOD("sample", "sample_pos"), &Voronoise::sample_bind);
}

Ref<Voronoise> Voronoise::create_bind(uint32_t p_grid_size, uint32_t p_map_seed, Vector2i p_map_offset) {
    Ref<Voronoise> vo;
    vo.instantiate(p_map_seed);
    return vo;
}

float Voronoise::sample_bind(Vector2 p_sample_pos) const {
    float distance;
    Vector2i closest_cell;
    //sample(p_sample_pos, distance, closest_cell);
    return 0.0f;
}

void WorldGenerator::_bind_methods() {
    ClassDB::bind_static_method("WorldGenerator", D_METHOD("create", "seed"), &WorldGenerator::create_bind);
    ClassDB::bind_static_method("WorldGenerator", D_METHOD("noise", "pos", "derivatives"), &WorldGenerator::noise);
    ClassDB::bind_method(D_METHOD("generate_chunk", "position", "image"), &WorldGenerator::generate_chunk);
    ClassDB::bind_method(D_METHOD("has_chunk", "position"), &WorldGenerator::has_chunk);
}

WorldGenerator::WorldGenerator(int64_t p_seed) : seed(p_seed) {
    biome_map_x_noise.instantiate();
    biome_map_y_noise.instantiate();
    biome_map_x_noise->set_seed(p_seed);
    biome_map_y_noise->set_seed(p_seed);

    biome_map_x_noise->set_fractal_type(FastNoiseLite::FRACTAL_NONE);
    biome_map_y_noise->set_fractal_type(FastNoiseLite::FRACTAL_NONE);
    biome_map_x_noise->set_noise_type(FastNoiseLite::TYPE_SIMPLEX);
    biome_map_y_noise->set_noise_type(FastNoiseLite::TYPE_PERLIN);
    biome_map_x_noise->set_frequency(0.01);
    biome_map_y_noise->set_frequency(0.3);

    biome_boundaries_noise.instantiate(seed);

    biome_boundary_distortion_noise.instantiate();
    biome_boundary_distortion_noise->set_noise_type(FastNoiseLite::TYPE_PERLIN);
    biome_boundary_distortion_noise->set_seed(p_seed);
    biome_boundary_distortion_noise->set_frequency(0.5);

    continentalness_curve.instantiate();
    continentalness_curve->clear_points();
    continentalness_curve->add_point(Vector2(0.0, 0.0f));
    continentalness_curve->add_point(Vector2(1.0, 1.0f));
}
