#ifndef LAYER_DEBUGER_H
#define LAYER_DEBUGER_H

#include "scene/main/node.h"
#include "worldgen/layer_system/layer_manager.h"

class ChunkerDebugger : public Node {
    GDCLASS(ChunkerDebugger, Node);

    ChunkerLayerManager *layer_manager = nullptr;
    size_t selected_layer = 0;
    float terrain_stochastic_scale = 1.0f;
    float terrain_stochastic_contrast = 1.0f;
    float terrain_stochastic_grid_scale = 1.0f;
protected:
    static void _bind_methods();
    void _draw_ui();
    void _notification(int p_what);
public:
    void set_layer_manager(ChunkerLayerManager *p_layer_manager);
};

#endif // LAYER_DEBUGER_H
