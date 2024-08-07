#include "register_types.h"

#include "core/config/project_settings.h"
#include "core/object/class_db.h"
#include "worldgen/chunker.h"
#include "worldgen/heightmap.h"
#include "poisson_disk_sampling.h"
#include "worldgen/quadtree.h"
#include "worldgen/road_astar.h"
#include "worldgen/worldgen_scatter.h"
#include "worldgen_manager.h"
#include "worldgen_composer.h"
#include "worldgen_debug.h"

void initialize_worldgen_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    GDREGISTER_ABSTRACT_CLASS(Voronoise);
    GDREGISTER_ABSTRACT_CLASS(WorldGenerator);
    GDREGISTER_ABSTRACT_CLASS(ChunkerQuadTree);
    GDREGISTER_CLASS(WorldgenDebug);
    GDREGISTER_CLASS(WorldgenManager);
    GDREGISTER_CLASS(WorldgenSettings);
    GDREGISTER_CLASS(RoadNetworkGenerator);
    GDREGISTER_CLASS(WorldgenBilinearArray);
    GDREGISTER_CLASS(ChunkerQuadTreeSettings);
    GDREGISTER_CLASS(WorldgenScatterSettings);
    GDREGISTER_CLASS(WorldgenScatterScene);
    GDREGISTER_CLASS(WorldgenScatterLayer);
    GDREGISTER_CLASS(WorldgenScatter);

    GLOBAL_DEF("kgame/chunk_size", 32);
    GLOBAL_DEF("kgame/cull_quadtree_subdiv", 2);
    GLOBAL_DEF("kgame/chunk_render_distance", 2);
    GLOBAL_DEF("kgame/chunk_unload_margin", 1);
    GLOBAL_DEF("kgame/grass_density", 2);
    GLOBAL_DEF("kgame/grass_subchunk_count", 2);
    GLOBAL_DEF("kgame/road_sdf_count", 64);
    GLOBAL_DEF("kgame/road_sdf_dimensions", 128);
    GLOBAL_DEF("kgame/chunk_heightmap_size", 128);
    GLOBAL_DEF("kgame/megachunk_radius", 1);
    GLOBAL_DEF("kgame/megachunk_size", 4096);
    GLOBAL_DEF(PropertyInfo(Variant::OBJECT, "kgame/height_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), Ref<Curve>());
}

void uninitialize_worldgen_module(ModuleInitializationLevel p_level) {
    
}
