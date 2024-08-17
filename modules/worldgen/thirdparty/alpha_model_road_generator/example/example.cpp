#include "alpha_model.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

void write_vertices(const std::vector<RoadGraph::Vertex> &p_vertices, const std::string &p_filename) {
std::ofstream file(p_filename);
if (!file) {
std::cerr << "Error opening file: " << p_filename << std::endl;
return;
}

for (int i = 0; i < p_vertices.size(); i++) {
file << p_vertices[i].point.x << "," << p_vertices[i].point.y << "," << p_vertices[i].mass << std::endl;
}

file.close();
}

void write_edges(const std::vector<RoadGraph::Edge> &p_edges, const std::string &p_filename) {
  std::ofstream file(p_filename);
  if (!file) {
    std::cerr << "Error opening file: " << p_filename << std::endl;
    return;
  }

  for (int i = 0; i < p_edges.size(); i++) {
    file << p_edges[i].v1 << "," << p_edges[i].v2 << std::endl;
  }

  file.close();
}

int main() {
  std::vector<RoadGraph::Vertex> cities;
  cities.push_back({.point = RoadGraph::Point {.x= 0.5, .y = 0.5}, .mass = 0.26});
  cities.push_back({.point = RoadGraph::Point {.x= 0.86, .y = 0.85}, .mass = 0.50});
  cities.push_back({.point = RoadGraph::Point {.x= 0.16, .y = 0.9}, .mass = 0.45});
  cities.push_back({.point = RoadGraph::Point {.x= 0.1, .y = 0.25}, .mass = 0.7});
  cities.push_back({.point = RoadGraph::Point {.x= 0.6, .y = 0.7}, .mass = 0.4});
  cities.push_back({.point = RoadGraph::Point {.x= 0.23, .y = 0.2}, .mass = 0.1});
  cities.push_back({.point = RoadGraph::Point {.x= 0.15, .y = 0.13}, .mass = 0.5});
  cities.push_back({.point = RoadGraph::Point {.x= 0.88, .y = 0.3}, .mass = 0.25});
  cities.push_back({.point = RoadGraph::Point {.x= 0.1, .y = 0.5}, .mass = 0.35});
  cities.push_back({.point = RoadGraph::Point {.x= 0.9, .y = 0.43}, .mass = 5.0});

  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

  AlphaModelRoadGenerator road_generator = AlphaModelRoadGenerator({}, cities);
  AlphaModelRoadGenerator::RoadGenerationOutput output;
  road_generator.generate_roads({
    .road_straightening_factor = 1.0f
  }, output);

  std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

  std::chrono::milliseconds seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Generation took " << seconds.count() << "ms" << std::endl;

  write_vertices(output.vertices, "vertices.csv");
  write_edges(output.edges, "edges.csv");
}