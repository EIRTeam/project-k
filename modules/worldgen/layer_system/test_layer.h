#ifndef TEST_LAYER_H
#define TEST_LAYER_H

#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "worldgen/layer_system/layer_manager.h"
#include "worldgen/layer_system/terrain_layers.h"
class TestLayer : public RefCounted {
    GDCLASS(TestLayer, RefCounted);
    Ref<ChunkerLayerManager> chunker;
    TestLayer() {
        chunker.instantiate();
        Ref<HeightmapLayer> heightmap_layer;
        heightmap_layer.instantiate();

        Ref<RoadLayer> road_layer;
        road_layer.instantiate();

        chunker->insert_layer(SNAME("Heightmap"), heightmap_layer);
        chunker->insert_layer(SNAME("Road SDF"), road_layer);
        chunker->add_layer_dependency(SNAME("Heightmap"), SNAME("Road SDF"));
    }

    void test() {
        chunker->update(Rect2(Vector2(0, 0), Vector2(1024, 1024)));
    }

    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("test"), &TestLayer::test);
    }
};

class TestManager : public RefCounted {
    GDCLASS(TestManager, RefCounted);

    Ref<ChunkerLayerManager> chunker;
    
    void update_camera_position(Vector2 p_camera_position) {
        const float render_distance = GLOBAL_GET("kgame/render_distance");
        const float half_render_distance = render_distance * 0.5f;
        Rect2 request_rect = Rect2(p_camera_position - Vector2(half_render_distance, half_render_distance), Vector2(render_distance, render_distance));
        chunker->update(request_rect);
    }
};

#endif // TEST_LAYER_H
