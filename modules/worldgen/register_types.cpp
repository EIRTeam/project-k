#include "register_types.h"

#include "core/config/project_settings.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/variant/variant.h"
#include "game_debugger.h"
#include "layer_system/layer_debuger.h"
#include "scene/resources/material.h"
#include "worldgen/chunker.h"
#include "worldgen/bilinear_array.h"
#include "poisson_disk_sampling.h"
#include "worldgen/heightmap_processor.h"
#include "worldgen/layer_system/biome_layers.h"
#include "worldgen/layer_system/test_layer.h"
#include "worldgen/quadtree.h"
#include "worldgen/road_astar.h"
#include "worldgen/vehicle/vehicle.h"
#include "worldgen/vehicle/vehicle_clutch.h"
#include "worldgen/vehicle/vehicle_debugger.h"
#include "worldgen/vehicle/vehicle_engine.h"
#include "worldgen/vehicle/vehicle_settings.h"
#include "worldgen/voronoi.h"
#include "wind/wind_gpu.h"
#include "worldgen/worldgen_height.h"

void initialize_worldgen_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    GDREGISTER_ABSTRACT_CLASS(ChunkerQuadTree);
    GDREGISTER_CLASS(VoronoiGraph);
    GDREGISTER_CLASS(BilinearVector);
    GDREGISTER_CLASS(ChunkerQuadTreeSettings);
    GDREGISTER_CLASS(HeightmapProcessor);
    GDREGISTER_CLASS(WindProcessor);

    GDREGISTER_CLASS(VehicleEngine);
    GDREGISTER_CLASS(VehicleSettings);
    GDREGISTER_CLASS(VehicleEngineSettings);
    GDREGISTER_CLASS(VehicleDifferentialSettings);
    GDREGISTER_CLASS(VehicleSuspensionCornerSettings);
    GDREGISTER_CLASS(VehicleSuspensionCorner);
    GDREGISTER_ABSTRACT_CLASS(VehicleTireSettings);
    GDREGISTER_CLASS(VehicleCurveModelTireSettings);
    GDREGISTER_CLASS(VehicleDrivetrainSettings);
    GDREGISTER_CLASS(HBVehicle);
    GDREGISTER_CLASS(VehicleDebugger);
    GDREGISTER_CLASS(VehicleClutch);
    GDREGISTER_CLASS(TestManager);
    GDREGISTER_CLASS(WorldgenHeightSettings);
    GDREGISTER_CLASS(BiomeGeneratorSettings);
    GDREGISTER_CLASS(BiomeSettings);
    GDREGISTER_CLASS(GameDebugger);
    GDREGISTER_CLASS(ChunkerDebugger);

    GLOBAL_DEF("kgame/chunk_size", 32);
    GLOBAL_DEF("kgame/cull_quadtree_subdiv", 2);
    GLOBAL_DEF("kgame/chunk_render_distance", 2);
    GLOBAL_DEF("kgame/chunk_unload_margin", 1);
    GLOBAL_DEF("kgame/road_sdf_count", 64);
    GLOBAL_DEF("kgame/road_sdf_dimensions", 512);
    GLOBAL_DEF("kgame/chunk_heightmap_size", 128);
    GLOBAL_DEF("kgame/render_distance", 2048.0f);

	GLOBAL_DEF(PropertyInfo(Variant::STRING, "kgame/terrain/terrain_base_material", PROPERTY_HINT_FILE, "*.tres,*.res"), "");
	GLOBAL_DEF(PropertyInfo(Variant::STRING, "kgame/terrain/terrain_shader", PROPERTY_HINT_FILE, "*.tres,*.res,*.gdshaderinc"), "");
	GLOBAL_DEF(PropertyInfo(Variant::STRING, "kgame/terrain/height_settings", PROPERTY_HINT_FILE, "*.tres,*.res"), "");
	GLOBAL_DEF(PropertyInfo(Variant::STRING, "kgame/terrain/biome_settings", PROPERTY_HINT_FILE, "*.tres,*.res"), "");
    
    GLOBAL_DEF("kgame/terrain/normal_height_texture_size", 128);
    GLOBAL_DEF(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "kgame/terrain/lod_max_distances"), PackedFloat32Array());
    GLOBAL_DEF(PropertyInfo(Variant::PACKED_INT32_ARRAY, "kgame/terrain/normal_height_texture_count_per_lod"), PackedInt32Array());
    GLOBAL_DEF("kgame/terrain/normal_epsilon", 1.0f);
    GLOBAL_DEF("kgame/terrain/terrain_chunk_size", 1024.0f);


    GLOBAL_DEF("kgame/wandering_heightmap/texture_size", 256);
    GLOBAL_DEF("kgame/wandering_heightmap/physical_size", 100);
    GLOBAL_DEF("kgame/wandering_heightmap/far", 100.0);

    GLOBAL_DEF("kgame/wind/windmap_radius", 1000.0);
    GLOBAL_DEF("kgame/wind/windmap_resolution", 512);
    GLOBAL_DEF("kgame/roads/road_width", 10.0f);
    GLOBAL_DEF("kgame/roads/road_skirt", 5.0f);


    
    GLOBAL_DEF(PropertyInfo(Variant::OBJECT, "kgame/height_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve"), Ref<Curve>());
}

void uninitialize_worldgen_module(ModuleInitializationLevel p_level) {
    
}
