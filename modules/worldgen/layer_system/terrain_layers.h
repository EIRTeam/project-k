#ifndef TERRAIN_LAYERS_H
#define TERRAIN_LAYERS_H

#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "layer_manager.h"
#include "worldgen/chunker.h"
#include "worldgen/bilinear_array.h"
#include "../thirdparty/taskflow/core/task.hpp"
#include "../thirdparty/taskflow/core/taskflow.hpp"
#include "worldgen/roads/road_network_generator.h"
#include "worldgen/worldgen_manager.h"
#include "../thirdparty/taskflow/algorithm/for_each.hpp"


class RoadChunk : public ChunkerChunk {
    Ref<WorldgenBilinearArray> road_sdf_array;
    int road_dimensions;
public:
    RoadChunk() {
        road_dimensions = GLOBAL_GET("kgame/road_sdf_dimensions");
    }

    virtual void build(tf::Taskflow &p_taskflow) override {
        tf::Task allocate_task = p_taskflow.emplace([&]() {
            road_sdf_array = WorldgenBilinearArray::create_xy(road_dimensions);
        }).name("Allocate road array");
        tf::Task generate_task = p_taskflow.for_each_index(0, road_dimensions*road_dimensions, 1, [&](int i) {
            Vector2 progress = Vector2(i % road_dimensions, Math::floor((float)i / road_dimensions)) / Vector2(road_dimensions-1, road_dimensions-1);
            Vector2i pixel_xy = Vector2i(i % road_dimensions, i / road_dimensions);
            Vector2 sample_pos = bounds.position + (progress * bounds.size);
            //float height = height_source->get_height(sample_pos);
            float height = 1.0f;
            road_sdf_array->set_pixel(pixel_xy, height);
        }).name("Generate road map");

        allocate_task.precede(generate_task);
    }
};

class RoadLayer : public ChunkerLayer {
    RoadLayer() {
    }
public:
    virtual float get_chunk_size() const override {
        return 1024.0f;
    }
    virtual Ref<ChunkerChunk> create_chunk() const override {
        Ref<RoadChunk> chunk;
        chunk.instantiate();
        return chunk;
    }
};

class HeightmapChunk : public ChunkerChunk {
    Ref<WorldgenHeight> height_source;
    int heightmap_dimensions;
    Ref<WorldgenBilinearArray> heightmap_array;
    Ref<Image> heightmap_image;
public:
    HeightmapChunk() {
        heightmap_dimensions = GLOBAL_GET("kgame/megachunk_heightmap_size");
        heightmap_array = WorldgenBilinearArray::create_xy(heightmap_dimensions);
        heightmap_image = Image::create_empty(heightmap_dimensions, heightmap_dimensions, false, Image::FORMAT_RGBAF);
    }
    virtual void build(tf::Taskflow &p_taskflow) override {
        tf::Task allocate_task = p_taskflow.emplace([&]() {
            heightmap_array->create_xy(heightmap_dimensions);
        }).name("Allocate heightmap array");
        tf::Task generate_task = p_taskflow.for_each_index(0, heightmap_dimensions*heightmap_dimensions, 1, [&](int i) {
            Vector2 progress = Vector2(i % heightmap_dimensions, Math::floor((float)i / heightmap_dimensions)) / Vector2(heightmap_dimensions-1, heightmap_dimensions-1);
            Vector2i pixel_xy = Vector2i(i % heightmap_dimensions, i / heightmap_dimensions);
            Vector2 sample_pos = bounds.position + ( progress * bounds.size);
            //float height = height_source->get_height(sample_pos);
            float height = 1.0f;
            heightmap_array->set_pixel(pixel_xy, height);
            heightmap_image->set_pixelv(pixel_xy, Color(progress.x, 0.0f, 0.0f, height));
        }).name("Generate heightmap");
        tf::Task upload_task = p_taskflow.emplace([&]() {
            heightmap_image->save_png("res://test.png");
        }).name("Upload heightmap to the GPU");

        allocate_task.precede(generate_task);
        generate_task.precede(upload_task);
    }
};

class HeightmapLayer : public ChunkerLayer {
    virtual float get_chunk_size() const override {
        return 2048.0f;
    }
    virtual Ref<ChunkerChunk> create_chunk() const override {
        Ref<HeightmapChunk> chunk;
        chunk.instantiate();
        return chunk;
    }
};

#endif // TERRAIN_LAYERS_H
