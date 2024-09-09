#ifndef INSTANCE_TEXTURE_QUEUE_H
#define INSTANCE_TEXTURE_QUEUE_H

#include "core/io/image.h"
#include "core/object/ref_counted.h"
#include "core/string/string_name.h"
#include "scene/resources/image_texture.h"

class InstanceTextureHandle;

class InstanceTextureQueue : public RefCounted {
    Ref<Texture2DArray> textures;

    List<int> available_uniform_indices;
    int image_count;
    int texture_dimensions;
public:
    StringName uniform_name;
    struct InstanceTextureQueueCreateParams {
        int texture_count = 2;
        Size2i texture_dimensions = Size2i(1, 1);
        Image::Format format = Image::Format::FORMAT_RGBA8;
        bool use_mipmaps = false;
        bool uses_global_uniform = false;
        StringName uniform_name;
    };

    Ref<InstanceTextureHandle> get_available_handle();
    InstanceTextureQueue(InstanceTextureQueueCreateParams p_create_params);
    void upload_image(int p_idx, Ref<Image> p_image) const;
    void release_idx(int p_idx);
    Ref<Texture2DArray> get_texture() const;
    int get_texture_dimensions() const;
};

class InstanceTextureHandle : public RefCounted {
    Ref<InstanceTextureQueue> queue;
    int idx;
public:
    int get_idx() const;
    void upload_image(Ref<Image> p_image) const;
    int get_texture_dimensions() const;
    InstanceTextureHandle(Ref<InstanceTextureQueue> p_queue, int p_idx);
    ~InstanceTextureHandle();
};

#endif // INSTANCE_TEXTURE_QUEUE_H
