#ifndef ROAD_NETWORK_GENERATOR_H
#define ROAD_NETWORK_GENERATOR_H

#include "alpha_model.h"
#include "core/object/ref_counted.h"
#include "core/typedefs.h"
#include "worldgen/roads/quadtree_road.h"
#include "../worldgen_height.h"
class WorldgenHeight;
class RoadNetworkGenerator;

class AlphaModelRoadGeneratorWithHeight : public AlphaModelRoadGenerator {
    Ref<RoadNetworkGenerator> network_generator;
    virtual float get_height(float p_x, float p_y) const override;
public:
    AlphaModelRoadGeneratorWithHeight() {};
    AlphaModelRoadGeneratorWithHeight(Ref<RoadNetworkGenerator> p_network_generator);
};

class RoadNetworkGenerator : public RefCounted {
    GDCLASS(RoadNetworkGenerator, RefCounted);
public:
    struct RoadNetworkSettings {
        float road_network_margin = 0.1f;
        Rect2 bounds;
    };
private:
    RoadNetworkSettings settings;
    AlphaModelRoadGeneratorWithHeight alpha_model;
    Ref<GridRoad> grid_road;
    Ref<WorldgenHeight> height;
    float sample_height(Vector2 p_position) const;
public:
    RoadNetworkGenerator(RoadNetworkSettings p_settings, Ref<WorldgenHeight> p_height_provider);

    _FORCE_INLINE_ Vector2 map_world_to_alpha(const Vector2 &p_world_position) const;
    _FORCE_INLINE_ Vector2 map_alpha_to_world(const Vector2 &p_alpha_position) const;

    float get_distance_to_road(Vector2 p_position) const;
    bool get_distance_to_road_clamped(const Vector2 &p_position, const float &p_max_distance_squared, float &r_distance) const;
    bool get_closest_road_point(const Vector2 &p_position, const float &p_max_distance_squared, float &r_distance, Vector2 &r_point) const;
    friend class AlphaModelRoadGeneratorWithHeight;
};

#endif // ROAD_NETWORK_GENERATOR_H
