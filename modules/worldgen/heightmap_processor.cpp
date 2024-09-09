#include "heightmap_processor.h"
#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/variant/typed_array.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/viewport.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_data_rd.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/rendering_device_commons.h"
#include "servers/rendering_server.h"
#include "worldgen/render_layers.h"

void HeightmapProcessor::_notification(int p_what) {
    switch(p_what) {
        case NOTIFICATION_READY: {
            if (Engine::get_singleton()->is_editor_hint()) {
                return;
            }


            top_down_camera = memnew(Camera3D);
            viewport = memnew(SubViewport);

            viewport->add_child(top_down_camera);
            top_down_camera->set_current(true);
            top_down_camera->set_global_basis(Basis().rotated(Vector3(1.0f, 0.0f, 0.0f), Math::deg_to_rad(-90.0f)));
            add_child(viewport);

            uint32_t heightmap_texture_size = GLOBAL_GET("kgame/wandering_heightmap/texture_size");
            
            viewport->set_size(Vector2i(heightmap_texture_size, heightmap_texture_size));
            viewport->set_clear_mode(SubViewport::CLEAR_MODE_NEVER);
            viewport->set_update_mode(SubViewport::UPDATE_ALWAYS);
            viewport->set_debug_draw(Viewport::DEBUG_DRAW_UNSHADED);

            int camera_far = GLOBAL_GET("kgame/wandering_heightmap/far");
            int camera_side = GLOBAL_GET("kgame/wandering_heightmap/physical_size");
            top_down_camera->set_orthogonal(camera_side, 0.0f, camera_far);

            top_down_camera->set_cull_mask(RENDER_LAYER_TERRAIN);

            compositor_effect_data.compositor_effect = RenderingServer::get_singleton()->compositor_effect_create();
            RS::get_singleton()->call_on_render_thread(callable_mp(this, &HeightmapProcessor::_init_render));
        } break;
    }
}

void HeightmapProcessor::_render_callback(const RenderingServer::CompositorEffectCallbackType &p_callback_type, RenderDataRD *p_render_data) {
    RenderSceneDataRD *scene_data = Object::cast_to<RenderSceneDataRD>(p_render_data->get_render_scene_data());
    
    DEV_ASSERT(scene_data != nullptr);
    Ref<RenderSceneBuffersRD> scene_buffers = p_render_data->get_render_scene_buffers();
    DEV_ASSERT(scene_buffers.is_valid());

    RenderingDevice *rd = RenderingDevice::get_singleton();

    // Update camera matrix buffer
    RendererRD::MaterialStorage::store_camera(scene_data->get_cam_projection().inverse(), compositor_effect_data.camera_matrices_buffer.inv_projection);
    RendererRD::MaterialStorage::store_transform(scene_data->get_cam_transform(), compositor_effect_data.camera_matrices_buffer.inv_view);
    rd->buffer_update(compositor_effect_data.camera_matrices_buffer_rd, 0, sizeof(compositor_effect_data.camera_matrices_buffer), &compositor_effect_data.camera_matrices_buffer);

    UniformSetCacheRD *uniform_set_cache = UniformSetCacheRD::get_singleton();

    Vector<RID> input_ids;
    input_ids.push_back(compositor_effect_data.sampler);
    input_ids.push_back(scene_buffers->get_depth_texture());

    RD::Uniform u_uniform_input = RD::Uniform(RenderingDeviceCommons::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE, 0, input_ids);
    RD::Uniform u_uniform_out = RD::Uniform(RenderingDeviceCommons::UNIFORM_TYPE_IMAGE, 1, compositor_effect_data.output_texture);
    RD::Uniform u_camera_matrices = RD::Uniform(RenderingDeviceCommons::UNIFORM_TYPE_UNIFORM_BUFFER, 2, compositor_effect_data.camera_matrices_buffer_rd);
    
    RID uniform_set = uniform_set_cache->get_cache(compositor_effect_data.shader, 0, u_uniform_input, u_uniform_out, u_camera_matrices);

    RD::ComputeListID cl = rd->compute_list_begin();
    rd->compute_list_bind_compute_pipeline(cl, compositor_effect_data.pipeline);
    rd->compute_list_set_push_constant(cl, &compositor_effect_data.push_constant, sizeof(compositor_effect_data.push_constant));
    rd->compute_list_bind_uniform_set(cl, uniform_set, 0);
    rd->compute_list_dispatch(cl, compositor_effect_data.x_groups, compositor_effect_data.y_groups, compositor_effect_data.z_groups);
    rd->compute_list_end();
}

void HeightmapProcessor::_init_render() {
    uint32_t heightmap_texture_size = GLOBAL_GET("kgame/wandering_heightmap/texture_size");
    RenderingServer *rs = RenderingServer::get_singleton();
    
    rs->compositor_effect_set_callback(compositor_effect_data.compositor_effect, RenderingServer::COMPOSITOR_EFFECT_CALLBACK_TYPE_POST_OPAQUE, callable_mp(this, &HeightmapProcessor::_render_callback));

    RD::TextureFormat texture_format = RD::TextureFormat();

    texture_format.format = RenderingDeviceCommons::DATA_FORMAT_R32_SFLOAT;
    texture_format.width = heightmap_texture_size;
    texture_format.height = heightmap_texture_size;
    texture_format.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT;

    RenderingDevice *rd = RenderingDevice::get_singleton();
    // create RD data
    compositor_effect_data.output_texture = rd->texture_create(texture_format, RD::TextureView());

    rd->set_resource_name(compositor_effect_data.output_texture, "Wandering heightmap output texture");

    compositor_effect_data.camera_matrices_buffer_rd = rd->uniform_buffer_create(sizeof(CameraMatricesBuffer));
    compositor_effect_data.shader = rd->shader_create_from_spirv(shader_file->get_spirv_stages(), "Wandering heightmap shader");
    compositor_effect_data.pipeline = rd->compute_pipeline_create(compositor_effect_data.shader);
    
    compositor_effect_data.push_constant.output_dimensions[0] = heightmap_texture_size;
    compositor_effect_data.push_constant.output_dimensions[1] = heightmap_texture_size;

    compositor_effect_data.sampler = rd->sampler_create(RD::SamplerState());

    compositor_effect_data.x_groups = (heightmap_texture_size - 1) / 8 + 1;
    compositor_effect_data.y_groups = (heightmap_texture_size - 1) / 8 + 1;
    compositor_effect_data.z_groups = 1;

    heightmap_texture_2d.instantiate();
    heightmap_texture_2d->set_texture_rd_rid(compositor_effect_data.output_texture);

    compositor_effect_data.compositor = rs->compositor_create();
    
    TypedArray<RID> effects;
    effects.push_back(compositor_effect_data.compositor_effect);
    
    rs->compositor_set_compositor_effects(compositor_effect_data.compositor, effects);
    rs->camera_set_compositor(top_down_camera->get_camera(), compositor_effect_data.compositor);

    rs->global_shader_parameter_set("heightmap", heightmap_texture_2d);
}

void HeightmapProcessor::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_shader_file"), &HeightmapProcessor::get_shader_file);
    ClassDB::bind_method(D_METHOD("set_shader_file", "shader_file"), &HeightmapProcessor::set_shader_file);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "shader_file", PROPERTY_HINT_RESOURCE_TYPE, "RDShaderFile"), "set_shader_file", "get_shader_file");
    ClassDB::bind_method(D_METHOD("update_camera_position", "camera_position"), &HeightmapProcessor::update_camera_position);
    ClassDB::bind_method(D_METHOD("get_output_texture"), &HeightmapProcessor::get_output_texture);
}

HeightmapProcessor::~HeightmapProcessor() {
    RenderingServer *rs = RenderingServer::get_singleton();
    RenderingDevice *rd = RenderingDevice::get_singleton();

    if (compositor_effect_data.output_texture.is_valid()) {
        rd->free(compositor_effect_data.output_texture);
    }
    if (compositor_effect_data.pipeline.is_valid()) {
        rd->free(compositor_effect_data.pipeline);
    }
    if (compositor_effect_data.shader.is_valid()) {
        rd->free(compositor_effect_data.shader);
    }
    if (compositor_effect_data.sampler.is_valid()) {
        rd->free(compositor_effect_data.sampler);
    }
    if (compositor_effect_data.camera_matrices_buffer_rd.is_valid()) {
        rd->free(compositor_effect_data.camera_matrices_buffer_rd);
    }
    if (compositor_effect_data.compositor_effect.is_valid()) {
        rs->free(compositor_effect_data.compositor_effect);
    }
    if (compositor_effect_data.compositor.is_valid()) {
        rs->free(compositor_effect_data.compositor);
    }
}

Ref<RDShaderFile> HeightmapProcessor::get_shader_file() const { return shader_file; }

void HeightmapProcessor::set_shader_file(const Ref<RDShaderFile> &p_shader_file) { shader_file = p_shader_file; }

Ref<Texture2D> HeightmapProcessor::get_output_texture() const {
    return heightmap_texture_2d;
};

void HeightmapProcessor::update_camera_position(const Vector3 &p_camera_position) {
    top_down_camera->set_global_position(p_camera_position + Vector3(0.0, 50.0, 0.0));
}

