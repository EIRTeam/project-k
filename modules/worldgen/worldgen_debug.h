#ifndef WORLDGEN_DEBUG_H
#define WORLDGEN_DEBUG_H

#include "scene/3d/node_3d.h"

class MultiMeshInstance3D;
class MeshInstance3D;
class WorldgenManager;
class MultiMesh;

class WorldgenDebug : public Node3D {
    GDCLASS(WorldgenDebug, Node3D);

    enum CrossSectionMode {
        CROSS_SECTION_VERTICAL,
        CROSS_SECTION_HORIZONTAL
    };

    bool picking_chunk = false;

    WorldgenManager *worldgen_manager = nullptr;
    Vector2i selected_chunk;
    Vector2i hovered_chunk;
    bool chunk_state_debug_enabled = false;
    bool draw_frustum_enabled = false;
    bool quadtree_drawing_enabled = false;


    CrossSectionMode cross_section_mode = CrossSectionMode::CROSS_SECTION_HORIZONTAL;
    Ref<Texture2D> chunk_height_texture;
    Ref<StandardMaterial3D> debug_material;
    static constexpr int CROSS_SECTION_RESOLUTION = 64;
    float cross_section_data[CROSS_SECTION_RESOLUTION] = {};
    float cross_section_depth = 0.0f;
    MeshInstance3D *chunk_pos_indicator = nullptr;
    MeshInstance3D *chunk_slice_indicator = nullptr;
    MeshInstance3D *frustum_mi = nullptr;
    MeshInstance3D *quad_tree_bounds_mi = nullptr;

protected:
    static void _bind_methods();
public:
    void _update_cross_section_data();

    void _draw_cross_section_ui();
    void set_manager(WorldgenManager *p_world_manager);

    void process_chunk_pick();
    void _notification(int p_what);

    WorldgenDebug();
};

#endif // WORLDGEN_DEBUG_H
