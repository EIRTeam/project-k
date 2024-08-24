#ifndef ROAD_LAYER_H
#define ROAD_LAYER_H

#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "core/string/print_string.h"
#include "layer_manager.h"
#include "../bilinear_array.h"
#include "worldgen/instance_texture_queue.h"
#include "worldgen/layer_system/terrain_layers.h"
class RoadLayer;
class RoadChunk : public ChunkerChunk {
    GDCLASS(RoadChunk, ChunkerChunk);
    Ref<BilinearVector> road_sdf_array;
    Ref<Image> road_sdf_image;
    Ref<Image> heightmap_image;
    int road_dimensions;
    int heightmap_dimensions;
    Ref<HeightmapLayer> heightmap_layer;
    Ref<InstanceTextureHandle> texture_handle;
    Ref<InstanceTextureHandle> height_texture_handle;
public:
    RoadChunk() {
        road_dimensions = GLOBAL_GET("kgame/road_sdf_dimensions");
        heightmap_dimensions = GLOBAL_GET("kgame/terrain_normal_height_texture_size");
    }

    virtual void build(tf::Taskflow &p_taskflow) override {
        tf::Task allocate_task = p_taskflow.emplace([&]() {
            road_sdf_array = BilinearVector::create_xy(road_dimensions);
            road_sdf_image = Image::create_empty(road_dimensions, road_dimensions, false, Image::FORMAT_RH);
            heightmap_image = Image::create_empty(heightmap_dimensions, heightmap_dimensions, false, Image::FORMAT_RGBAH);
        }).name("Allocate road array and image");
        tf::Task generate_task = p_taskflow.for_each_index(0, road_dimensions*road_dimensions, 1, [&](int i) {
            Vector2 progress = Vector2(i % road_dimensions, Math::floor((float)i / road_dimensions)) / Vector2(road_dimensions-1, road_dimensions-1);
            Vector2i pixel_xy = Vector2i(i % road_dimensions, i / road_dimensions);
            Vector2 sample_pos = bounds.position + (progress * bounds.size);
            float height = heightmap_layer->sample_height_at_position(sample_pos);
            road_sdf_array->set_pixel(pixel_xy, height);
            road_sdf_image->set_pixelv(pixel_xy, Color(height, 0.0f, 0.0f, 0.0f));

        }).name("Generate road map");
        tf::Task generate_heightmap_task = p_taskflow.for_each_index(0, heightmap_dimensions*heightmap_dimensions, 1, [&] (int i){
            Vector2i pixel_xy = Vector2i(i % heightmap_dimensions, i / heightmap_dimensions);
            Vector2 progress = Vector2(i % heightmap_dimensions, Math::floor((float)i / heightmap_dimensions)) / Vector2(heightmap_dimensions-1, heightmap_dimensions-1);
            Vector2 sample_pos = bounds.position + (progress * bounds.size);
            float height;
            Vector2 derivative;
            heightmap_layer->sample_height_with_derivative_at_position(sample_pos, 1.0f, height, derivative);
            heightmap_image->set_pixelv(pixel_xy, Color(derivative.x, derivative.y, 0.0, height));
        }).name("Generate heightmap");
        tf::Task upload_task = p_taskflow.emplace([&]() {
            //texture_handle->upload_image(road_sdf_image);
            height_texture_handle->upload_image(heightmap_image);
        }).name("Upload to the GPU");

        allocate_task.precede(generate_task);
        generate_task.precede(generate_heightmap_task);
        generate_heightmap_task.precede(upload_task);
    }
    Ref<InstanceTextureHandle> get_texture_handle() const {
        return texture_handle;
    }
    Ref<InstanceTextureHandle> get_heightmap_texture_handle() const {
        return height_texture_handle;
    }
    friend class RoadLayer;
};

class RoadLayer : public ChunkerLayer {
    GDCLASS(RoadLayer, ChunkerLayer);
    Ref<InstanceTextureQueue> texture_queue;
    Ref<InstanceTextureQueue> heightmap_texture_queue;
    Ref<HeightmapLayer> heightmap_layer;
public:
    RoadLayer(Ref<HeightmapLayer> p_heightmap_layer) {
        heightmap_layer = p_heightmap_layer;
        const int road_dimensions = GLOBAL_GET("kgame/road_sdf_dimensions");
        texture_queue.instantiate(InstanceTextureQueue::InstanceTextureQueueCreateParams {
            .texture_count = 4,
            .texture_dimensions = Vector2i(road_dimensions, road_dimensions),
            .format = Image::FORMAT_RH,
            .uses_global_uniform = true,
            .uniform_name = SNAME("road_sdfs")
        });
        const int height_texture_dimensions = GLOBAL_GET("kgame/terrain_normal_height_texture_size");
        heightmap_texture_queue.instantiate(InstanceTextureQueue::InstanceTextureQueueCreateParams {
            .texture_count = 128,
            .texture_dimensions = Vector2i(height_texture_dimensions, height_texture_dimensions),
            .format = Image::FORMAT_RGBAH,
            .uses_global_uniform = true,
            .uniform_name = SNAME("terrain_normal_heightmaps")
        });
    }
    virtual float get_chunk_size() const override {
        return 2048.0f;
    }
    virtual float get_chunk_padding() const override {
        return 32.0f;
    }
    virtual Ref<ChunkerChunk> create_chunk() const override {
        Ref<RoadChunk> chunk;
        chunk.instantiate();
        chunk->heightmap_layer = heightmap_layer;
        //chunk->texture_handle = texture_queue->get_available_handle();
        chunk->height_texture_handle = heightmap_texture_queue->get_available_handle();
        return chunk;
    }
};

#endif // ROAD_LAYER_H
