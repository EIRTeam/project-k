#include "biome_layers.h"
#include "core/object/class_db.h"
#include "core/object/object.h"

void BiomeSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_reference_height"), &BiomeSettings::get_reference_height);
    ClassDB::bind_method(D_METHOD("set_reference_height", "reference_height"), &BiomeSettings::set_reference_height);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_height"), "set_reference_height", "get_reference_height");

    ClassDB::bind_method(D_METHOD("get_height_multiplier"), &BiomeSettings::get_height_multiplier);
    ClassDB::bind_method(D_METHOD("set_height_multiplier", "height_multiplier"), &BiomeSettings::set_height_multiplier);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_multiplier"), "set_height_multiplier", "get_height_multiplier");

    ClassDB::bind_method(D_METHOD("get_selector_rect"), &BiomeSettings::get_selector_rect);
    ClassDB::bind_method(D_METHOD("set_selector_rect", "selector_rect"), &BiomeSettings::set_selector_rect);
    ADD_PROPERTY(PropertyInfo(Variant::RECT2, "selector_rect"), "set_selector_rect", "get_selector_rect");

    ClassDB::bind_method(D_METHOD("get_noise"), &BiomeSettings::get_noise);
    ClassDB::bind_method(D_METHOD("set_noise", "noise"), &BiomeSettings::set_noise);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_noise", "get_noise");
}

void BiomeGeneratorSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_x_noise"), &BiomeGeneratorSettings::get_x_noise);
    ClassDB::bind_method(D_METHOD("set_x_noise", "x_noise"), &BiomeGeneratorSettings::set_x_noise);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "x_noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_x_noise", "get_x_noise");

    ClassDB::bind_method(D_METHOD("get_y_noise"), &BiomeGeneratorSettings::get_y_noise);
    ClassDB::bind_method(D_METHOD("set_y_noise", "y_noise"), &BiomeGeneratorSettings::set_y_noise);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "y_noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_y_noise", "get_y_noise");

    ClassDB::bind_method(D_METHOD("get_biomes"), &BiomeGeneratorSettings::get_biomes_bind);
    ClassDB::bind_method(D_METHOD("set_biomes", "biomes"), &BiomeGeneratorSettings::set_biomes_bind);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "biomes", PROPERTY_HINT_ARRAY_TYPE, "BiomeSettings"), "set_biomes", "get_biomes");

}

Ref<ChunkerChunk> BiomeVoronoiTriangulationLayer::create_chunk(int p_lod_level) const {
    Ref<BiomeVoronoiTriangulationChunk> chunk;
    chunk.instantiate();
    chunk->layer = const_cast<BiomeVoronoiTriangulationLayer*>(this);
    chunk->point_layer = points_layer;
    return chunk;
}
