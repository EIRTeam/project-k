#ifndef ALPHA_MODEL_H
#define ALPHA_MODEL_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <vector>

#ifdef ALPHA_MODEL_DEBUG
#include <iostream>
#endif

class RoadGraph {
public:
    struct Point {
        double x = 0.0f;
        double y = 0.0f;
        double length() const {
            return sqrt(x*x + y*y);
        }

        Point normalized() const {
            double len = length();
            if (len == 0.0) {
                return Point();
            }
            return Point {
                .x = x / len,
                .y = y / len
            };
        }

        bool is_zero() const {
            return x == 0.0 && y == 0.0;
        }
    };
    struct Vertex {
        Point point;
        double mass = 0.0f;
        bool is_city = false;
    };

    struct Edge {
        int v1, v2; // Indices of vertices
        double length;
    };

    std::vector<Vertex> vertices;
    std::vector<Edge> edges;
    std::vector<std::vector<int>> adjacent_edges;
public:
    void insert_vertex(Vertex p_vertex);
    void reserve_vertices(std::size_t p_vertex_count);
    void reserve_edges(std::size_t p_edge_count);
    void insert_edge(Edge p_edge);
    int get_vertex_count() const;

    std::vector<int> pathfind(int p_a_node, int p_b_node, const std::vector<std::vector<float>>& p_weight, float p_alpha);

    const std::vector<Vertex> &get_vertices() const;
    const std::vector<Edge> &get_edges() const;
    const std::vector<std::vector<int>> &get_adjacents() const;
};

class AlphaModelRoadGenerator {
public:
    struct AlphaModelRoadGeneratorSettings {
        float bounds_start_x = 0.1f;
        float bounds_start_y = 0.1f;
        float bounds_end_x = 0.9f;
        float bounds_end_y = 0.9f;
        int dummy_point_count = 6000;
    };
private:
    RoadGraph road_graph;
    AlphaModelRoadGeneratorSettings settings;
    std::vector<RoadGraph::Vertex> cities;
public:
    AlphaModelRoadGenerator(AlphaModelRoadGeneratorSettings p_settings, const std::vector<RoadGraph::Vertex> &p_cities);

    struct RoadGenerationOutput {
        std::vector<RoadGraph::Vertex> vertices;
        std::vector<RoadGraph::Edge> edges;
        std::vector<std::vector<int>> adjacency_list;
    };

    struct RoadGenerationSettings {
        float road_straightening_factor = 0.9f;
        float alpha = 0.7f;
    };
    void generate_roads(const RoadGenerationSettings p_settings, RoadGenerationOutput &p_output);
};


#endif // ALPHA_MODEL_H
