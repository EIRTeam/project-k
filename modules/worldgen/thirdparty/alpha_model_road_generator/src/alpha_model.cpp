#include "alpha_model.h"
#include <algorithm>
#include <limits>
#include <queue>
#include <unordered_map>
#include "delaunator.hpp"

void RoadGraph::insert_vertex(Vertex p_vertex) {
    vertices.emplace_back(p_vertex);
    adjacent_edges.emplace_back(std::vector<int>());
}

void RoadGraph::reserve_vertices(std::size_t p_vertex_count) {
    vertices.reserve(p_vertex_count);
    adjacent_edges.reserve(p_vertex_count);
}

void RoadGraph::reserve_edges(std::size_t p_edge_count) {
    edges.reserve(p_edge_count);
}

void RoadGraph::insert_edge(Edge p_edge) {
    double x_diff = (vertices[p_edge.v1].point.x - vertices[p_edge.v2].point.x);
    double y_diff = (vertices[p_edge.v1].point.y - vertices[p_edge.v2].point.y);
    p_edge.length = x_diff*x_diff + y_diff*y_diff;
    edges.emplace_back(p_edge);
    adjacent_edges[p_edge.v1].push_back(edges.size()-1);
    adjacent_edges[p_edge.v2].push_back(edges.size()-1);
}

int RoadGraph::get_vertex_count() const {
    return vertices.size();
}

// Dijkstra-based pathfinding function
std::vector<int> RoadGraph::pathfind(int p_a_node, int p_b_node, const std::vector<std::vector<float>>& p_weight, float p_alpha) {
    // Priority queue to store (cost, vertex index), sorted by lowest cost
    using PQElement = std::pair<double, int>;
    std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> frontier;

    // Cost to reach each node
    std::vector<double> cost_so_far(vertices.size(), std::numeric_limits<double>::infinity());
    cost_so_far[p_a_node] = 0;

    // To reconstruct the path
    std::vector<int> came_from(vertices.size(), -1);

    // Start with the initial node
    frontier.emplace(0.0, p_a_node);

    while (!frontier.empty()) {
        int current = frontier.top().second;
        frontier.pop();

        // If we reached the goal
        if (current == p_b_node) {
            break;
        }

        // Explore neighbors
        for (int edge_index : adjacent_edges[current]) {
            const Edge& edge = edges[edge_index];
            int next_node = (edge.v1 == current) ? edge.v2 : edge.v1;

            float coeff = 1.0f;

            if (p_weight[current][next_node] > 0.0f) {
                coeff = p_alpha;
            }

            float weight = (edge.length * edge.length) * coeff;

            double new_cost = cost_so_far[current] + weight;

            // If a cheaper path to the next node is found
            if (new_cost < cost_so_far[next_node]) {
                cost_so_far[next_node] = new_cost;
                frontier.emplace(new_cost, next_node);
                came_from[next_node] = current;
            }
        }
    }

    // Reconstruct the path from the `came_from` map
    std::vector<int> path;
    if (came_from[p_b_node] == -1) {
        return path;  // No path found
    }

    for (int current = p_b_node; current != p_a_node; current = came_from[current]) {
        path.push_back(current);
    }
    path.push_back(p_a_node);
    std::reverse(path.begin(), path.end());

    return path;
}

const std::vector<RoadGraph::Vertex> &RoadGraph::get_vertices() const {
    return vertices;
}

const std::vector<RoadGraph::Edge> &RoadGraph::get_edges() const {
    return edges;
}

const std::vector<std::vector<int>> &RoadGraph::get_adjacents() const {
    return adjacent_edges;
}

struct HaltonPoint {
  double x, y;
};

// Function to calculate the radical inverse of an integer in base b
double radical_inverse(int p_n, int p_base) {
  double result = 0.0;
  double f = 1.0 / p_base;
  while (p_n > 0) {
    result += f * (p_n % p_base);
    p_n /= p_base;
    f /= p_base;
  }
  return result;
}

// Function to generate Halton sequence points in a unit square
std::vector<HaltonPoint> generate_halton_points(int numPoints, int base1, int base2) {
  std::vector<HaltonPoint> points;
  for (int i = 1; i <= numPoints; ++i) {
    double x = radical_inverse(i, base1);
    double y = radical_inverse(i, base2);
    points.push_back({x, y});
  }
  return points;
}

// Function to map points to a bounding box
std::vector<HaltonPoint> map_to_bounding_box(const std::vector<HaltonPoint> &points,
                                    double minX, double minY, double maxX,
                                    double maxY) {
  std::vector<HaltonPoint> mappedPoints;
  for (const HaltonPoint &p : points) {
    double x = minX + p.x * (maxX - minX);
    double y = minY + p.y * (maxY - minY);
    mappedPoints.push_back({x, y});
  }
  return mappedPoints;
}
AlphaModelRoadGenerator::AlphaModelRoadGenerator(AlphaModelRoadGeneratorSettings p_settings, const std::vector<RoadGraph::Vertex> &p_cities) {
    settings = p_settings;

    for (const RoadGraph::Vertex &v : p_cities) {
        int eq_count = std::count_if(p_cities.begin(), p_cities.end(), [&](const RoadGraph::Vertex &p_vertex) {
            return p_vertex.point.x == v.point.x && p_vertex.point.y == v.point.y;
        });
        if (eq_count > 1) {
            continue;
        }

        cities.push_back(v);
        road_graph.insert_vertex(v);
    }

    // Generate dummy points
    {
        std::vector<double> coords;
        coords.reserve((p_settings.dummy_point_count * 2) + (cities.size() * 2));

        for (const RoadGraph::Vertex &city : cities) {
            coords.push_back(city.point.x);
            coords.push_back(city.point.y);
        }

        std::vector<HaltonPoint> dummy_points = generate_halton_points(settings.dummy_point_count, 2, 3);
        dummy_points = map_to_bounding_box(dummy_points, settings.bounds_start_x, settings.bounds_start_y, settings.bounds_end_x, settings.bounds_end_y);
        for (const HaltonPoint &p_point : dummy_points) {
            coords.emplace_back(p_point.x);
            coords.emplace_back(p_point.y);
            road_graph.insert_vertex({
                .point = RoadGraph::Point {
                    .x = p_point.x,
                    .y = p_point.y
                }
            });
        }

        delaunator::Delaunator delaunator(coords);
        road_graph.reserve_edges(delaunator.triangles.size() * 3);
        for (int i = 0; i < delaunator.triangles.size(); i+=3) {
            road_graph.insert_edge({
                .v1 = static_cast<int>(delaunator.triangles[i]),
                .v2 = static_cast<int>(delaunator.triangles[i+1])
            });
            road_graph.insert_edge({
                .v1 = static_cast<int>(delaunator.triangles[i+1]),
                .v2 = static_cast<int>(delaunator.triangles[i+2])
            });
            road_graph.insert_edge({
                .v1 = static_cast<int>(delaunator.triangles[i+2]),
                .v2 = static_cast<int>(delaunator.triangles[i])
            });
        }
    }
}

void AlphaModelRoadGenerator::generate_roads(RoadGenerationSettings p_settings, RoadGenerationOutput &p_output) {
    
    struct NumberOfTrips {
        int city_a;
        int city_b;
        double number_of_trips;
    };

    std::vector<NumberOfTrips> number_of_trips;
    number_of_trips.reserve(cities.size() * (cities.size()-1) / 2);
    for (int i = 0; i < cities.size(); i++) {
        for (int j = i+1; j < cities.size(); j++) {
            double x_diff = (cities[i].point.x - cities[j].point.x);
            double y_diff = (cities[i].point.y - cities[j].point.y);
            float d = x_diff*x_diff + y_diff*y_diff;
            number_of_trips.emplace_back(NumberOfTrips {
                .city_a = i,
                .city_b = j,
                .number_of_trips = cities[i].mass * cities[j].mass / (1.0f + d)
            });
        }
    }

    // stable_sort for repeatability
    std::stable_sort(number_of_trips.begin(), number_of_trips.end(), [](const NumberOfTrips &p_a, const NumberOfTrips &p_b) {
        return p_a.number_of_trips > p_b.number_of_trips;
    });
    
    const int vertex_count = road_graph.get_vertex_count();
    std::vector<std::vector<float>> is_road(vertex_count, std::vector<float>(vertex_count, 0.0f));

    for (int i = 0; i < number_of_trips.size(); i++) {
        std::vector<int> path = road_graph.pathfind(number_of_trips[i].city_a, number_of_trips[i].city_b, is_road, p_settings.alpha);
        for (int j = 0; j < path.size()-1; j++) {
            int curr = path[j];
            int next = path[j+1];
            is_road[curr][next] += number_of_trips[i].number_of_trips;
            is_road[next][curr] += number_of_trips[i].number_of_trips;
        }
    }

    std::unordered_map<int, int> vertex_remap;

    const std::vector<RoadGraph::Vertex> &vertices = road_graph.get_vertices();
    const std::vector<std::vector<int>> &adjacency_list = road_graph.get_adjacents();
    const std::vector<RoadGraph::Edge> &edge_list = road_graph.get_edges();

    for (int i = 0; i < vertex_count; i++) {
        for (int adjacent : adjacency_list[i]) {
            const RoadGraph::Edge &edge = edge_list[adjacent];
            if (is_road[edge.v1][edge.v2] == 0.0f) {
                continue;
            }

            // We must keep this vertex
            p_output.vertices.push_back(vertices[i]);
            vertex_remap.emplace(i, p_output.vertices.size()-1);
            break;
        }
    }

    p_output.adjacency_list.resize(p_output.vertices.size());

    for (int i = 0; i < edge_list.size(); i++) {
        const RoadGraph::Edge &edge = edge_list[i];
        if (is_road[edge.v1][edge.v2] == 0.0f) {
            continue;
        }
        int new_v1 = vertex_remap[edge.v1];
        int new_v2 = vertex_remap[edge.v2];
        p_output.edges.emplace_back(
            RoadGraph::Edge {
                .v1 = new_v1,
                .v2 = new_v2,
                .length = edge.length
            }
        );
        p_output.adjacency_list[new_v1].push_back(p_output.edges.size()-1);
        p_output.adjacency_list[new_v2].push_back(p_output.edges.size()-1);
    }

    // relax edges
    for (int i = 0; i < p_output.vertices.size(); i++) {
        const RoadGraph::Vertex &vertex = p_output.vertices[i];
        if (vertex.is_city || p_output.adjacency_list[i].size() == 0) {
            continue;
        }

        double average_x = 0.0f;
        double average_y = 0.0f;


        for (int adjacent : p_output.adjacency_list[i]) {
            int other = p_output.edges[adjacent].v1 == i ? p_output.edges[adjacent].v2 : p_output.edges[adjacent].v1;
            average_x += p_output.vertices[other].point.x;
            average_y += p_output.vertices[other].point.y;
        }

        average_x /= (double)p_output.adjacency_list[i].size();
        average_y /= (double)p_output.adjacency_list[i].size();
        
        RoadGraph::Point diff = {
            .x = average_x - p_output.vertices[i].point.x,
            .y = average_y - p_output.vertices[i].point.y
        };

        if (diff.length() == 0.0) {
            continue;
        }
        double len = diff.length();
        double distance_to_move = len * std::clamp(p_settings.road_straightening_factor, 0.0f, 1.0f);
        RoadGraph::Point dir = diff.normalized();
        p_output.vertices[i].point.x += dir.x * distance_to_move;
        p_output.vertices[i].point.y += dir.y * distance_to_move;
    }
}
