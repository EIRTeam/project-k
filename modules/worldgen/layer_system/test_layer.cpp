#include "test_layer.h"

void TestManager::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            chunker = memnew(ChunkerLayerManager);
            add_child(chunker);




            const StringName heightmap_layer_name = SNAME("Heightmap Layer");
            const StringName quadtree_layer_name = SNAME("Terrain QuadTree");
            const StringName road_layer_name = SNAME("Road SDF");
            heightmap_layer.instantiate();
            road_layer.instantiate(heightmap_layer);
            quadtree_layer.instantiate(road_layer);

            chunker->insert_layer(quadtree_layer_name, quadtree_layer);
            chunker->insert_layer(heightmap_layer_name, heightmap_layer);
            chunker->insert_layer(road_layer_name, road_layer);

            chunker->add_layer_dependency(road_layer_name, heightmap_layer_name);
            chunker->add_layer_dependency(quadtree_layer_name, road_layer_name);

            set_process(true);
        } break;
        case NOTIFICATION_PROCESS: {
            Camera3D *cam = get_viewport()->get_camera_3d();
            if (cam) {
                const Vector3 cam_pos = cam->get_global_position();
                update_camera_position(Vector2(cam_pos.x, cam_pos.z));
            }
        } break;
    }
}

void TestManager::update_camera_position(Vector2 p_camera_position) {
    const float render_distance = GLOBAL_GET("kgame/render_distance");
    const float half_render_distance = render_distance * 0.5f;
    Rect2 request_rect = Rect2(p_camera_position - Vector2(half_render_distance, half_render_distance), Vector2(render_distance, render_distance));
    chunker->update(request_rect);
    quadtree_layer->set_camera_position(p_camera_position);
    quadtree_layer->update_terrain_chunks();
}
