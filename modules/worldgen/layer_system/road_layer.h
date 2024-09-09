#ifndef ROAD_LAYER_H
#define ROAD_LAYER_H

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/image.h"
#include "core/string/print_string.h"
#include "core/variant/variant.h"
#include "layer_manager.h"
#include "../bilinear_array.h"
#include "worldgen/instance_texture_queue.h"
#include "heightmap_layer.h"
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
    }

    virtual void build(tf::Taskflow &p_taskflow) override {
        heightmap_dimensions = height_texture_handle->get_texture_dimensions();
        tf::Task allocate_task = p_taskflow.emplace([&]() {
            road_sdf_array = BilinearVector::create_xy(road_dimensions);
            road_sdf_image = Image::create_empty(road_dimensions, road_dimensions, false, Image::FORMAT_RH);
            heightmap_image = Image::create_empty(heightmap_dimensions, heightmap_dimensions, false, Image::FORMAT_RH);
        }).name("Allocate road array and image");
        tf::Task generate_task = p_taskflow.for_each_index(0, road_dimensions*road_dimensions, 1, [&](int i) {
            Vector2 progress = Vector2(i % road_dimensions, Math::floor((float)i / road_dimensions)) / Vector2(road_dimensions-1, road_dimensions-1);
            Vector2i pixel_xy = Vector2i(i % road_dimensions, i / road_dimensions);
            Vector2 sample_pos = bounds.position + (progress * bounds.size);
            float height = heightmap_layer->sample_height_at_position(sample_pos);
            road_sdf_image->set_pixelv(pixel_xy, Color(height, 0.0f, 0.0f, 0.0f));

        }).name("Generate road map");
        tf::Task generate_heightmap_task = p_taskflow.for_each_index(0, heightmap_dimensions*heightmap_dimensions, 1, [&] (int i){
            Vector2i pixel_xy = Vector2i(i % heightmap_dimensions, i / heightmap_dimensions);
            Vector2 progress = Vector2(i % heightmap_dimensions, Math::floor((float)i / heightmap_dimensions)) / Vector2(heightmap_dimensions-2, heightmap_dimensions-2);
            Vector2 sample_pos = bounds.position + (progress * bounds.size);
            float height = heightmap_layer->sample_height_at_position(sample_pos);
            heightmap_image->set_pixelv(pixel_xy, Color(height, 0.0, 0.0, 1.0));
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
    LocalVector<Ref<InstanceTextureQueue>> heightmap_texture_queues;
    Ref<HeightmapLayer> heightmap_layer;
    PackedInt32Array per_lod_heightmap_dimensions;
public:
    RoadLayer(Ref<HeightmapLayer> p_heightmap_layer) {
        heightmap_layer = p_heightmap_layer;
        const PackedFloat32Array lod_max_distances =  GLOBAL_GET("kgame/terrain/lod_max_distances");
        const int height_texture_dimensions = GLOBAL_GET("kgame/terrain/normal_height_texture_size");
        const PackedInt32Array texture_count_per_lod = GLOBAL_GET("kgame/terrain/normal_height_texture_count_per_lod");

        DEV_ASSERT(lod_max_distances.size() == texture_count_per_lod.size());

        for (int i = 0; i < lod_max_distances.size(); i++) {
            const int texture_dimension = height_texture_dimensions/MAX(1, 2 * i);
            const int texture_count = texture_count_per_lod[i];
            per_lod_heightmap_dimensions.push_back(texture_dimension);
            Ref<InstanceTextureQueue> texture_queue;
            const StringName shader_parameter_name = vformat("terrain_normal_heightmaps_lod_%d", i);
            RenderingServer::get_singleton()->global_shader_parameter_add(shader_parameter_name, RenderingServer::GLOBAL_VAR_TYPE_SAMPLER2DARRAY, Variant());

            texture_queue.instantiate(InstanceTextureQueue::InstanceTextureQueueCreateParams {
                .texture_count = texture_count,
                .texture_dimensions = Vector2i(texture_dimension, texture_dimension),
                .format = Image::FORMAT_RH,
                .uses_global_uniform = true,
                .uniform_name = shader_parameter_name
            });
            heightmap_texture_queues.push_back(texture_queue);
        }
    }
    virtual float get_chunk_size() const override {
        return GLOBAL_GET("kgame/terrain/terrain_chunk_size");
    }
    virtual float get_chunk_padding() const override {
        return 32.0f;
    }
    virtual Ref<ChunkerChunk> create_chunk(int p_lod_level) const override {
        Ref<RoadChunk> chunk;
        chunk.instantiate();
        chunk->heightmap_layer = heightmap_layer;
        print_line("GRAB HANDLE FOR CHUNK LOD", p_lod_level);
        chunk->height_texture_handle = heightmap_texture_queues[p_lod_level]->get_available_handle();
        return chunk;
    }
};

#endif // ROAD_LAYER_H
