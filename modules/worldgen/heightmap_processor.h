#ifndef HEIGHTMAP_PROCESSOR_H
#define HEIGHTMAP_PROCESSOR_H

#include "scene/3d/node_3d.h"
#include "scene/main/node.h"
#include "scene/resources/texture.h"
#include "scene/resources/texture_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering_server.h"

class Camera3D;
class SubViewport;


class HeightmapProcessor : public Node {
    GDCLASS(HeightmapProcessor, Node);

    Camera3D *top_down_camera = nullptr;
    SubViewport *viewport = nullptr;
    Ref<Texture2DRD> heightmap_texture_2d;
    Ref<RDShaderFile> shader_file;

    struct CameraMatricesBuffer {
        float inv_projection[16];
        float inv_view[16];
    };

    struct PushConstant {
        float output_dimensions[2];
        float padding[2];
    };

    struct CompositorEffectData {
        int x_groups;
        int y_groups;
        int z_groups;
        RID pipeline;
        RID compositor;
        RID compositor_effect;
        RID output_texture;
        RID camera_matrices_buffer_rd;
        RID shader;
        RID sampler;
        CameraMatricesBuffer camera_matrices_buffer;
        PushConstant push_constant;
    } compositor_effect_data;

    void _notification(int p_what);
    void _render_callback(const RenderingServer::CompositorEffectCallbackType &p_callback_type, RenderDataRD *p_render_data);

protected:
    static void _bind_methods();

    ~HeightmapProcessor();

public:
    Ref<RDShaderFile> get_shader_file() const;
    void set_shader_file(const Ref<RDShaderFile> &p_shader_file);
    Ref<Texture2D> get_output_texture() const;
    void update_camera_position(const Vector3 &p_camera_position);
};

#endif // HEIGHTMAP_PROCESSOR_H
