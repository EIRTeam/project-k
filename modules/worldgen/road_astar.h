#ifndef ROAD_ASTAR_H
#define ROAD_ASTAR_H

#include "core/io/file_access.h"
#include "core/math/a_star.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"
#include "core/templates/safe_refcount.h"
#include "core/typedefs.h"
#include "worldgen/heightmap.h"
#include "worldgen/quadtree.h"
class RoadAStar : public AStar2D {
    GDCLASS(RoadAStar, AStar2D);
public:

	Vector<float> point_heights;

	enum RoadAStarNodeConnectionMode {
		WIDE,
		NORMAL
	};
    struct RoadAStarSettings {
        Ref<WorldgenBilinearArray> heightmap;
		RoadAStarNodeConnectionMode connection_mode = RoadAStarNodeConnectionMode::NORMAL;
		int astar_resolution = 8;
		float power = 1.5f;
		float multiplier = 10.0f;
		int chunk_size;
    };

	RoadAStarSettings settings;

	_FORCE_INLINE_ int pos_to_idx(const Vector2i &p_point) const {
		return (p_point.x) + (p_point.y * settings.astar_resolution);
    }
	
	_FORCE_INLINE_ Vector2i idx_to_pos(int p_idx) {
		return Vector2i((p_idx % settings.astar_resolution), (p_idx / settings.astar_resolution));
    }

	static Mutex flag;

    RoadAStar(RoadAStarSettings &p_settings) {
        settings = p_settings;

        reserve_space(settings.astar_resolution * settings.astar_resolution);
		point_heights.resize(settings.astar_resolution * settings.astar_resolution);

		float *point_heights_w = point_heights.ptrw();

        for (int i = 0; i < settings.astar_resolution * settings.astar_resolution; i++) {
            Vector2 pos = idx_to_pos(i);
			Vector2 heightmap_pos = pos / Vector2(settings.astar_resolution-1, settings.astar_resolution-1);
			add_point(i, heightmap_pos);
            float height = p_settings.heightmap->sample(heightmap_pos * p_settings.heightmap->get_dimension());
			point_heights_w[i] = height;
        }

        Rect2i rect = Rect2i(0, 0, settings.astar_resolution-1, settings.astar_resolution-1);

        for (int i = 0; i < get_point_count(); i++) {
            static const Vector2i direction_offsets[8] = {
				Vector2i(1, 0),
				Vector2i(-1, 0),
				Vector2i(0, 1),
				Vector2i(0, -1),

				Vector2i(-1, 1),
				Vector2i(-1, -1),
				Vector2i(1, 1),
				Vector2i(1, -1),
				
				//Vector2i(1, 2),
				//Vector2i(2, 1),
				//Vector2i(-1, 2),
				//Vector2i(-2, 1),
				//Vector2i(1, -2),
				//Vector2i(2, -1),
				//Vector2i(-1, -2),
				//Vector2i(-2, -1),
            };

			Vector2i pos = idx_to_pos(i);
            for (Vector2i dir : direction_offsets) {
				Vector2i sample_pos = pos+dir;
				if (sample_pos.x >= settings.astar_resolution || sample_pos.x < 0) {
					continue;
				}
				if (sample_pos.y >= settings.astar_resolution || sample_pos.y < 0) {
					continue;
				}
				int test_idx = i + dir.y * settings.astar_resolution + dir.x;
				if (i == 0) {
					print_line(pos, sample_pos, test_idx);
				}
				connect_points(i, test_idx, true);
            }
        }
		/*
		Ref<FileAccess> fa;
		if (flag.try_lock()) {
			fa = FileAccess::open("res://astardump.txt", FileAccess::WRITE);
			for(int i = 0; i < get_point_count(); i++) {
				fa->store_line(vformat("P%d %v", i, get_point_position(i)));
				Vector<int64_t> cn = get_point_connections(i);
				int64_t connection_count = get_point_connections(i).size();
				fa->store_string(vformat(" %d %s", connection_count, cn));
				fa->store_string("\n");
			}
			flag.unlock();
		}
		*/
    }

	float compute_octile_distance(const Vector2 &p_from, const Vector2 &p_to) {
		float dx = Math::abs(p_to.x - p_from.x);
		float dy = Math::abs(p_to.y - p_from.y);
		float f = 1.4142135623730950488016887242f-1.0f;
		return dx < dy ? f * dx + dy : f * dy + dx;
    }

    virtual real_t _compute_cost(int64_t p_from_id, int64_t p_to_id) override {
		Vector2i pos_from = get_point_position(p_from_id);
		Vector2i pos_to = get_point_position(p_to_id);
		float flat_distance = (pos_from*settings.chunk_size).distance_to(pos_to*settings.chunk_size);
		if (flat_distance == 0.0f) {
            return 0.0f;
        }
		float height_diff = Math::abs(
			point_heights[p_from_id] - point_heights[p_to_id]
		);
		float slope = height_diff / flat_distance;
		
		float octile_distance = flat_distance;
		return octile_distance * (1.0 + pow(slope*settings.multiplier, settings.power));
    }
    virtual real_t _estimate_cost(int64_t p_from_id, int64_t p_to_id) override {
        return _compute_cost(p_from_id, p_to_id);
    }

	Vector<Vector2> get_path(const Vector2 &p_from, const Vector2 &p_to) {
		// Takes and returns 0-1 positions
		Vector<Vector2> points = get_point_path(pos_to_idx(p_from * (settings.astar_resolution-1)), pos_to_idx(p_to * (settings.astar_resolution-1)));
		return points;
	}

	int get_astar_resolution() const {
		return settings.astar_resolution;
	}
};

#endif // ROAD_ASTAR_H
