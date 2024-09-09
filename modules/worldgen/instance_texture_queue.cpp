#include "instance_texture_queue.h"
#include "core/error/error_macros.h"
#include "scene/resources/image_texture.h"
#include "servers/rendering_server.h"
#include "scene/resources/image_texture.h"

Ref<InstanceTextureHandle> InstanceTextureQueue::get_available_handle() {
    ERR_FAIL_COND_V_MSG(available_uniform_indices.size() == 0, Ref<InstanceTextureHandle>(), String(uniform_name));
    int first = available_uniform_indices.front()->get();
    available_uniform_indices.pop_front();
    
    Ref<InstanceTextureHandle> handle;
    handle.instantiate(this, first);
    return handle;
}

InstanceTextureQueue::InstanceTextureQueue(InstanceTextureQueueCreateParams p_create_params) {
    image_count = p_create_params.texture_count;

    RS *rs = RS::get_singleton();
    Ref<Image> base_image = Image::create_empty(p_create_params.texture_dimensions.x, p_create_params.texture_dimensions.y, p_create_params.use_mipmaps, p_create_params.format);
    Vector<Ref<Image>> images;
    images.resize(p_create_params.texture_count);
    images.fill(base_image);
    textures.instantiate();
    textures->create_from_images(images);
    if (p_create_params.uses_global_uniform) {
        rs->global_shader_parameter_set(p_create_params.uniform_name, textures);
    }

    for(int i = 0; i < p_create_params.texture_count; i++) {
        available_uniform_indices.push_back(i);
    }

    texture_dimensions = p_create_params.texture_dimensions.x;
}

void InstanceTextureQueue::upload_image(int p_idx, Ref<Image> p_image) const {
    ERR_FAIL_INDEX(p_idx, image_count);
    textures->update_layer(p_image, p_idx);
}

void InstanceTextureQueue::release_idx(int p_idx) {
    available_uniform_indices.push_back(p_idx);
}

Ref<Texture2DArray> InstanceTextureQueue::get_texture() const {
    return textures;
}

int InstanceTextureQueue::get_texture_dimensions() const {
    return texture_dimensions;
}

int InstanceTextureHandle::get_idx() const {
    return idx;
}

void InstanceTextureHandle::upload_image(Ref<Image> p_image) const {
    queue->upload_image(idx, p_image);
}

int InstanceTextureHandle::get_texture_dimensions() const {
    return queue->get_texture_dimensions();
}

InstanceTextureHandle::InstanceTextureHandle(Ref<InstanceTextureQueue> p_queue, int p_idx) {
    queue = p_queue;
    idx = p_idx;
}

InstanceTextureHandle::~InstanceTextureHandle() {
    queue->release_idx(idx);
}

