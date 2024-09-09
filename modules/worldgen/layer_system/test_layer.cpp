#include "test_layer.h"
#include "worldgen/layer_system/layer_debuger.h"

void TestManager::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            chunker_debugger = memnew(ChunkerDebugger);
            add_child(chunker_debugger);

            chunker = memnew(ChunkerLayerManager);
            add_child(chunker);

            chunker_debugger->set_layer_manager(chunker);

            const StringName biome_voronoi_points_layer_name = SNAME("Biome Points");
            const StringName biome_voronoi_layer_name = SNAME("Biome Voronoi");
            const StringName heightmap_layer_name = SNAME("Heightmap Layer");
            const StringName quadtree_layer_name = SNAME("Terrain QuadTree");
            const StringName road_layer_name = SNAME("Road SDF");
            biome_point_layer.instantiate();
            biome_layer.instantiate(biome_point_layer);
            heightmap_layer.instantiate(biome_layer);
            road_layer.instantiate(heightmap_layer);
            quadtree_layer.instantiate(road_layer);

            chunker->insert_layer(quadtree_layer_name, quadtree_layer);
            chunker->insert_layer(heightmap_layer_name, heightmap_layer);
            chunker->insert_layer(road_layer_name, road_layer);
            chunker->insert_layer(biome_voronoi_layer_name, biome_layer);
            chunker->insert_layer(biome_voronoi_points_layer_name, biome_point_layer);

            chunker->add_layer_dependency(road_layer_name, heightmap_layer_name);
            chunker->add_layer_dependency(quadtree_layer_name, road_layer_name);
            chunker->add_layer_dependency(heightmap_layer_name, biome_voronoi_layer_name);
            chunker->add_layer_dependency(biome_voronoi_layer_name, biome_voronoi_points_layer_name);
            chunker->set_lod_max_distances(GLOBAL_GET("kgame/terrain/lod_max_distances"));

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
    chunker->update(request_rect, p_camera_position);
    quadtree_layer->set_camera_position(p_camera_position);
    quadtree_layer->update_terrain_chunks();
}
