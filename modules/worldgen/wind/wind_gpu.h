#ifndef WIND_PROCESSOR_H
#define WIND_PROCESSOR_H

#include "scene/3d/node_3d.h"
#include "scene/main/node.h"
#include "scene/resources/texture.h"
#include "scene/resources/texture_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering_server.h"

class Camera3D;
class SubViewport;

class WindProcessor : public Node {
    GDCLASS(WindProcessor, Node);

    Ref<RDShaderFile> shader_file;
    Ref<Texture2DRD> output_texture;
    float pixel_world_size;

    struct PushConstant {
        float camera_position[2];
        float output_dimensions[2];
        float offset[2];
        float physical_size;
        float windmap_scale = 0.01f;
    };

    struct GPUData {
        int x_groups;
        int y_groups;
        int z_groups;
        RID pipeline;
        RID output_texture;
        RID shader;
        PushConstant push_constant;
    } gpu_data;

    void _notification(int p_what);

protected:
    static void _bind_methods();

    ~WindProcessor();

public:
    void dispatch();
    Ref<RDShaderFile> get_shader_file() const;
    void set_shader_file(const Ref<RDShaderFile> &p_shader_file);
    Ref<Texture2D> get_output_texture() const;
    void update_camera_position(const Vector3 &p_camera_position);
    Vector2 get_windmap_center() const;
    void set_windmap_scale(float p_windmap_scale);
    void advance_wind(const Vector2 &p_offset);
    float get_windmap_scale() const;
};

#endif // WIND_PROCESSOR_H
