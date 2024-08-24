#ifndef TEST_LAYER_H
#define TEST_LAYER_H

#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/main/node.h"
#include "scene/main/viewport.h"
#include "layer_manager.h"
#include "quadtree_layer.h"
#include "road_layer.h"
#include "terrain_layers.h"

class TestManager : public Node3D {
    GDCLASS(TestManager, Node3D);

    ChunkerLayerManager *chunker = nullptr;
    Ref<QuadTreeTerrainLayer> quadtree_layer;
    Ref<HeightmapLayer> heightmap_layer;
    Ref<RoadLayer> road_layer;

    void _notification(int p_what);

    void update_camera_position(Vector2 p_camera_position);
};

#endif // TEST_LAYER_H
