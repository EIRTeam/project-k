#include "road_network_generator.h"
#include "alpha_model.h"
#include "core/math/geometry_2d.h"
#include "worldgen/roads/quadtree_road.h"
#include "scene/resources/curve.h"

float RoadNetworkGenerator::sample_height(Vector2 p_position) const {
    return height->get_height(map_alpha_to_world(p_position));
}

RoadNetworkGenerator::RoadNetworkGenerator(RoadNetworkSettings p_settings, Ref<WorldgenHeight> p_height_provider) {
    settings = p_settings;
    height = p_height_provider;

    struct CTee {
        Vector2 point;
        float mass;
    } ctees[10] = {
        {.point = Vector2(0.5, 0.5),  .mass = 0.26},
        {.point = Vector2(0.86, 0.85),  .mass = 0.50},
        {.point = Vector2(0.16, 0.9),  .mass = 0.45},
        {.point = Vector2(0.1, 0.25),  .mass = 0.7},
        {.point = Vector2(0.6, 0.7),  .mass = 0.4},
        {.point = Vector2(0.23, 0.2),  .mass = 0.1},
        {.point = Vector2(0.15, 0.13),  .mass = 0.5},
        {.point = Vector2(0.88, 0.3),  .mass = 0.25},
        {.point = Vector2(0.1, 0.5),  .mass = 0.35},
        {.point = Vector2(0.9, 0.43),  .mass = 5.0},
    };

    std::vector<RoadGraph::Vertex> cities;

    for (const CTee &ct : ctees) {
        Vector2 p = ct.point;
        cities.push_back({
            .point = {p.x, p.y},
            .mass = ct.mass
        });
    }

    alpha_model = AlphaModelRoadGeneratorWithHeight (this);

    alpha_model.initialize({
        .bounds_start_x = settings.road_network_margin,
        .bounds_start_y = settings.road_network_margin,
        .bounds_end_x = 1.0f - settings.road_network_margin,
        .bounds_end_y = 1.0f - settings.road_network_margin,
    }, cities);

    AlphaModelRoadGenerator::RoadGenerationOutput output;

    alpha_model.generate_roads({
        .road_straightening_factor = 1.0,
        .alpha = 0.7,
        .heightmap_weight = 0.1f
    }, output);

    grid_road = GridRoad::create({
       .bounds = p_settings.bounds 
    });


    for (size_t i = 0; i < output.edges.size(); i++) {
        RoadGraph::Point p1 = output.vertices[output.edges[i].v1].point;
        RoadGraph::Point p2 = output.vertices[output.edges[i].v2].point;
        Vector2 p11 = map_alpha_to_world(Vector2(p1.x, p1.y));
        Vector2 p22 = map_alpha_to_world(Vector2(p2.x, p2.y));
        grid_road->insert_segment(p11, p22);
    }
}

_FORCE_INLINE_ Vector2 RoadNetworkGenerator::map_world_to_alpha(const Vector2 &p_world_position) const {
    return (p_world_position - settings.bounds.position) / settings.bounds.size;
}

_FORCE_INLINE_ Vector2 RoadNetworkGenerator::map_alpha_to_world(const Vector2 &p_alpha_position) const {
    return (p_alpha_position * settings.bounds.size) + settings.bounds.position;
}

float RoadNetworkGenerator::get_distance_to_road(Vector2 p_position) const {
    // Remap position to alpha model coords
    float dist = 10000.0f;
    get_distance_to_road_clamped(p_position, 5000.0f * 5000.0f, dist);
    return dist;
}

bool RoadNetworkGenerator::get_distance_to_road_clamped(const Vector2 &p_position, const float &p_max_distance_squared, float &r_distance) const {
    Vector2 point;
    bool found = get_closest_road_point(p_position, Math::sqrt(p_max_distance_squared), r_distance, point);
    return found;
}

bool RoadNetworkGenerator::get_closest_road_point(const Vector2 &p_position, const float &p_max_distance_squared, float &r_distance, Vector2 &r_point) const {
    // Remap position to alpha model coords
    GridRoad::Segment closest;
    bool found = grid_road->sample_closest_segment(p_position, Math::sqrt(p_max_distance_squared), closest);
    Vector2 points[2] = {
        closest.from,
        closest.to,
    };
    Vector2 closest_point = Geometry2D::get_closest_point_to_segment(p_position, points);
    r_distance = closest_point.distance_to(p_position);
    r_point = closest_point;
    return found;
}

float AlphaModelRoadGeneratorWithHeight::get_height(float p_x, float p_y) const {
    return network_generator->sample_height(Vector2(p_x, p_y));
}

AlphaModelRoadGeneratorWithHeight::AlphaModelRoadGeneratorWithHeight(Ref<RoadNetworkGenerator> p_network_generator) {
    network_generator = p_network_generator;
}
