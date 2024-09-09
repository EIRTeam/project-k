#include "wind_gpu.h"
#include "core/config/project_settings.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "scene/main/node.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device_commons.h"
#include "servers/rendering_server.h"

void WindProcessor::_notification(int p_what) {
    switch(p_what) {
        case NOTIFICATION_READY: {
            if (!shader_file.is_valid()) {
                return;
            }

            RS::get_singleton()->call_on_render_thread(callable_mp(this, &WindProcessor::_init_render));            
        } break;
    }
}

void WindProcessor::_bind_methods() {
    ClassDB::bind_method(D_METHOD("dispatch"), &WindProcessor::dispatch);
    ClassDB::bind_method(D_METHOD("update_camera_position", "camera_position"), &WindProcessor::update_camera_position);
    ClassDB::bind_method(D_METHOD("get_shader_file"), &WindProcessor::get_shader_file);
    ClassDB::bind_method(D_METHOD("set_shader_file", "shader_file"), &WindProcessor::set_shader_file);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "shader_file", PROPERTY_HINT_RESOURCE_TYPE, "RDShaderFile"), "set_shader_file", "get_shader_file");

    ClassDB::bind_method(D_METHOD("get_windmap_scale"), &WindProcessor::get_windmap_scale);
    ClassDB::bind_method(D_METHOD("set_windmap_scale", "windmap_scale"), &WindProcessor::set_windmap_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "windmap_scale", PROPERTY_HINT_RANGE, "0.001,100,0.001"), "set_windmap_scale", "get_windmap_scale");
    ClassDB::bind_method(D_METHOD("get_output_texture"), &WindProcessor::get_output_texture);
    ClassDB::bind_method(D_METHOD("get_windmap_center"), &WindProcessor::get_windmap_center);
    ClassDB::bind_method(D_METHOD("advance_wind", "offset"), &WindProcessor::advance_wind);
}

WindProcessor::~WindProcessor() {
    RD *rd = RD::get_singleton();
    if (gpu_data.output_texture.is_valid()) {
        rd->free(gpu_data.output_texture);
    }
    if (gpu_data.pipeline.is_valid()) {
        rd->free(gpu_data.pipeline);
    }
    if (gpu_data.pipeline.is_valid()) {
        rd->free(gpu_data.shader);
    }
}

void WindProcessor::_init_render() {
    RD *rd = RD::get_singleton();

    gpu_data.shader = rd->shader_create_from_spirv(shader_file->get_spirv_stages(), "Wind GPU compute");
    
    RD::TextureFormat texture_format;
    int windmap_resolution = GLOBAL_GET("kgame/wind/windmap_resolution");
    texture_format.width = windmap_resolution;
    texture_format.height = windmap_resolution;
    texture_format.format = RenderingDeviceCommons::DATA_FORMAT_R16G16B16A16_SFLOAT;
    texture_format.usage_bits = RenderingDeviceCommons::TEXTURE_USAGE_STORAGE_BIT | RenderingDeviceCommons::TEXTURE_USAGE_SAMPLING_BIT;

    gpu_data.output_texture = rd->texture_create(texture_format, RD::TextureView());
    rd->set_resource_name(gpu_data.output_texture, "Wind map");

    gpu_data.pipeline = rd->compute_pipeline_create(gpu_data.shader);
    gpu_data.push_constant.output_dimensions[0] = windmap_resolution;
    gpu_data.push_constant.output_dimensions[1] = windmap_resolution;

    gpu_data.x_groups = (windmap_resolution - 1) / 8 + 1;
    gpu_data.y_groups = (windmap_resolution - 1) / 8 + 1;
    gpu_data.z_groups = 1;

    output_texture.instantiate();
    output_texture->set_texture_rd_rid(gpu_data.output_texture);
    RS::get_singleton()->global_shader_parameter_set("wind_map", output_texture);

    float windmap_radius = GLOBAL_GET("kgame/wind/windmap_radius");
    pixel_world_size =  (windmap_radius * 2.0) / (float)windmap_resolution;
    gpu_data.push_constant.physical_size = windmap_radius*2.0;
}

void WindProcessor::dispatch() {
    RD *rd = RD::get_singleton();
    
    rd->draw_command_begin_label("Wind");

    RD::Uniform u_input = RD::Uniform(RenderingDeviceCommons::UNIFORM_TYPE_IMAGE, 0, gpu_data.output_texture);
    UniformSetCacheRD *uniform_set_cache = UniformSetCacheRD::get_singleton();
    RID uniform_set = uniform_set_cache->get_cache(gpu_data.shader, 0, u_input);
    
    RD::ComputeListID cl = rd->compute_list_begin();
    rd->compute_list_bind_compute_pipeline(cl, gpu_data.pipeline);
    rd->compute_list_bind_uniform_set(cl, uniform_set, 0);
    rd->compute_list_set_push_constant(cl, &gpu_data.push_constant, sizeof(gpu_data.push_constant));
    rd->compute_list_dispatch(cl, gpu_data.x_groups, gpu_data.y_groups, gpu_data.z_groups);
    rd->compute_list_end();
    rd->draw_command_end_label();
    RS *rs = RS::get_singleton();
    rs->global_shader_parameter_set("wind_map_center", get_windmap_center());
    float windmap_radius = GLOBAL_GET("kgame/wind/windmap_radius");
    rs->global_shader_parameter_set("wind_map_side", windmap_radius);
}

Ref<RDShaderFile> WindProcessor::get_shader_file() const {
    return shader_file;
}

void WindProcessor::set_shader_file(const Ref<RDShaderFile> &p_shader_file) {
    shader_file = p_shader_file;
}

Ref<Texture2D> WindProcessor::get_output_texture() const {
    return output_texture;
}

void WindProcessor::update_camera_position(const Vector3 &p_camera_position) {
    Vector2 center = (Vector2(p_camera_position.x, p_camera_position.z) / pixel_world_size).floor()*pixel_world_size;
    center -= Vector2(pixel_world_size, pixel_world_size) * 0.5;
    gpu_data.push_constant.camera_position[0] = center.x;
    gpu_data.push_constant.camera_position[1] = center.y;
}

Vector2 WindProcessor::get_windmap_center() const {
    return Vector2(gpu_data.push_constant.camera_position[0], gpu_data.push_constant.camera_position[1]);
}

void WindProcessor::set_windmap_scale(float p_windmap_scale) {
    gpu_data.push_constant.windmap_scale = p_windmap_scale;
}

void WindProcessor::advance_wind(const Vector2 &p_offset) {
    gpu_data.push_constant.offset[0] += p_offset.x;
    gpu_data.push_constant.offset[1] += p_offset.y;
}

float WindProcessor::get_windmap_scale() const {
    return gpu_data.push_constant.windmap_scale;
};
