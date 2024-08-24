#ifndef QTREE_ROAD_H
#define QTREE_ROAD_H

#include "core/math/geometry_2d.h"
#include "core/math/plane.h"
#include "core/math/rect2.h"
#include "core/math/vector2i.h"
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/variant/variant.h"
#include "servers/rendering/renderer_scene_cull.h"
#include <queue>
#include <vector>

// Strategy (for now) is to keep a completely subdivided quad tree in memory, it might be too
// memory intensive, so perhaps we could change it later
class GridRoad : public RefCounted {
    GDCLASS(GridRoad, RefCounted);

public:
    struct QuadTreeRoadSettings {
        Rect2 bounds = Rect2(0, 0, 4096, 4096);
        int grid_element_count = 64;
    };
    float grid_element_size;
private:
    QuadTreeRoadSettings settings;

    Vector2i chunk_dimensions;
public:
    static Ref<GridRoad> create(QuadTreeRoadSettings p_settings) {
        Ref<GridRoad> road;
        road.instantiate();
        road->settings = p_settings;
        road->chunk_dimensions.x = p_settings.grid_element_count;
        road->chunk_dimensions.y = p_settings.grid_element_count;
        road->grid_element_size = p_settings.bounds.size.x / (float)p_settings.grid_element_count;
        road->data.resize(road->chunk_dimensions.x * road->chunk_dimensions.y);

        for (size_t i = 0; i < road->data.size(); i++) {
            Vector2i chunk_xy = Vector2i(i % p_settings.grid_element_count, i / p_settings.grid_element_count);
            Rect2 chunk_rect = Rect2(Vector2(chunk_xy) * road->grid_element_size, Vector2(road->grid_element_size, road->grid_element_size));
            road->data[i].bounds = chunk_rect;
        }

        return road;
    }

    struct Segment {
        Vector2 from;
        Vector2 to;
    };

    struct Chunk {
        Rect2 bounds;
        LocalVector<Segment> segments;
    };

    LocalVector<Chunk> data;

    Vector<int> get_chunks_in_rect(Rect2 p_bbox) const {
        int start_chunk_x = Math::floor(p_bbox.position.x / grid_element_size);
        start_chunk_x = MAX(0, start_chunk_x);
        int start_chunk_y = Math::floor(p_bbox.position.y / grid_element_size);
        start_chunk_y = MAX(0, start_chunk_y);
        int end_chunk_x = Math::ceil((p_bbox.position.x + p_bbox.size.x) / grid_element_size);
        end_chunk_x = MIN(chunk_dimensions.x, end_chunk_x+1);
        int end_chunk_y = Math::ceil((p_bbox.position.y + p_bbox.size.y) / grid_element_size);
        end_chunk_y = MIN(chunk_dimensions.y, end_chunk_y+1);
        Vector<int> out;
        for (int x = start_chunk_x; x < end_chunk_x; x++) {
            for (int y = start_chunk_y; y < end_chunk_y; y++) {
                int chunk_idx = x + y * chunk_dimensions.y;
                out.push_back(chunk_idx);
            }
        }
        return out;
    }

    void insert_segment(const Vector2 &p_start, const Vector2 &p_end) {
        Rect2 segment_bounding_box = Rect2(p_start, Vector2());
        segment_bounding_box.expand_to(p_end);
        Vector<int> chunks = get_chunks_in_rect(segment_bounding_box);
        for (const int &chunk_idx : chunks) {
            if (data[chunk_idx].bounds.intersects_segment(p_start, p_end)) {
                data[chunk_idx].segments.push_back({
                    .from = p_start,
                    .to = p_end
                });
            }
        }
    }

    bool sample_closest_segment(const Vector2 p_position, const float p_max_distance, Segment &r_segment) const {
        Vector<const Segment*> found_segments;
        std::priority_queue<std::pair<float, int> , std::vector<std::pair<float, int>>, std::greater<std::pair<float, int>>> distance_queue;
        Rect2 sample_bbox = Rect2(p_position - Vector2(p_max_distance, p_max_distance), Vector2(p_max_distance, p_max_distance) * 2.0f);
        Vector<int> chunks = get_chunks_in_rect(sample_bbox);
        for (const int &chunk_idx : chunks) {
            for  (const Segment &segment : data[chunk_idx].segments) {
                Vector2 segment_ptr[2] = {
                    segment.from,
                    segment.to,
                };
                Vector2 closest_point = Geometry2D::get_closest_point_to_segment(p_position, segment_ptr);
                distance_queue.emplace(std::make_pair(p_position.distance_squared_to(closest_point), found_segments.size()));
                found_segments.push_back(&segment);
            }
        }

        if (!distance_queue.empty()) {
            if (distance_queue.top().first < (p_max_distance * p_max_distance)) {
                r_segment = *found_segments[distance_queue.top().second];
                return true;
            }
        }

        return false;
    }
};

#endif // QTREE_ROAD_H
