#include "heightmap_layer.h"
#include "core/error/error_macros.h"
#include "core/string/print_string.h"
#include "worldgen/bilinear_array.h"

Ref<HeightmapChunk> HeightmapLayer::get_chunk_at_world_position(Vector2 p_world_position) const {
    const Vector2i chunk = Vector2(p_world_position / get_chunk_size()).floor();
    HashMap<Vector2i, Ref<ChunkerChunk>>::ConstIterator it = loaded_chunks.find(chunk);
    ERR_FAIL_COND_V_MSG(it == loaded_chunks.end(), Ref<HeightmapChunk>(), "Tried to get chunk at position that doesn't exist");
    return it->value;
}

float HeightmapLayer::sample_height_at_position(Vector2 p_world_position) const {
    Ref<HeightmapChunk> chunk = get_chunk_at_world_position(p_world_position);
    ERR_FAIL_COND_V(!chunk.is_valid(), 0.0f);
    Ref<WorldBoundBilinearArray> bilinear_array = chunk->heightmap_array;
    return bilinear_array->sample(p_world_position);
}

void HeightmapLayer::sample_height_with_derivative_at_position(Vector2 p_world_position, float p_dir_eps, float &r_height, Vector2 &r_derivative) const {
    float sample_height = sample_height_at_position(p_world_position);
    float h2 = sample_height_at_position(p_world_position + Vector2(p_dir_eps, 0.0f));
    float h3 = sample_height_at_position(p_world_position + Vector2(0.0f, p_dir_eps));

    r_height = sample_height;

    float slope_x = h2 - sample_height;
    float slope_z = h3 - sample_height;
    r_derivative.x = slope_x;
    r_derivative.y = slope_z;
}

Ref<ChunkerChunk> HeightmapLayer::create_chunk(int p_lod_level) const {
    Ref<HeightmapChunk> chunk;
    chunk.instantiate(biomes_layer);
    return chunk;
}

HeightmapLayer::HeightmapLayer(Ref<BiomeVoronoiTriangulationLayer> p_biomes_layer) {
    biomes_layer = p_biomes_layer;
}

float HeightmapLayer::get_chunk_size() const {
    return GLOBAL_GET("kgame/terrain/terrain_chunk_size");
}

float HeightmapLayer::get_chunk_padding() const {
    return 1024.0f;
}
