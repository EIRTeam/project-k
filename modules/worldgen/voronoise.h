#ifndef VORONOISE_H
#define VORONOISE_H

#include "core/object/ref_counted.h"
#include <array>
#include <algorithm>

class Voronoise : public RefCounted {
    GDCLASS(Voronoise, RefCounted);
    uint32_t map_seed;
public:
    struct Cell {
        Vector2i position;
        Vector2 center;
        Vector2 offset;
    };
private:
    HashMap<Vector2i, Cell> cell_data_cache;

    Vector2 get_cell_offset(Vector2i p_cell) {
    };



    void get_cell_data_cached(Vector2i p_pos, Cell &p_cell) const {
        HashMap<Vector2i, Cell>::ConstIterator it = cell_data_cache.find(p_pos);
        if (it == cell_data_cache.end()) {
            get_cell_data(p_pos, p_cell);
            const_cast<Voronoise*>(this)->cell_data_cache.insert(p_pos, p_cell);
        } else {
            p_cell = it->value;
        }
    }
public:

    struct VoronoiSampleResult {
        float closest_distances[9];
        Vector2i closest_cell_centers[9];
        Vector2 closest_cell_positions[9];
    };

    void get_cell_data(Vector2i p_pos, Cell &p_cell) const {
        uint64_t cell_seed = HashMapHasherDefault::hash(Vector3i(p_pos.x, p_pos.y, map_seed));
        float offset_x = (Math::rand_from_seed(&cell_seed) / (double)UINT32_MAX) - 0.5;
        float offset_y = (Math::rand_from_seed(&cell_seed) / (double)UINT32_MAX) - 0.5;
        p_cell.position = p_pos;
        p_cell.center = (Vector2)p_pos + Vector2(0.5, 0.5);
        p_cell.offset = Vector2(offset_x, offset_y);
    }

    void sample(const Vector2 &p_sample_position_map, VoronoiSampleResult &r_result) const {
        Vector2i pos_truncated = p_sample_position_map;

        static const Vector2i DIRECTIONS[9] {
            Vector2i(0, 0),
            Vector2i(1, 0),
            Vector2i(-1, 0),
            Vector2i(0, -1),
            Vector2i(0, 1),
            Vector2i(1, 1),
            Vector2i(-1, 1),
            Vector2i(-1, -1),
            Vector2i(1, -1),
        };

        struct CellDistance {
            Vector2i cell_center;
            Vector2 pos;
            float distance;
            size_t idx;
        };
        
        std::array<CellDistance, 9> distances;

        {
            for (size_t i = 0; i < std::size(distances); i++) {
                Vector2i neighbor_cell_pos = pos_truncated + DIRECTIONS[i];
                Cell cell;
                get_cell_data_cached(neighbor_cell_pos, cell);
                Vector2 cell_pos = cell.center + cell.offset;
                distances[i] = {
                    .cell_center = neighbor_cell_pos,
                    .pos = cell_pos,
                    .distance = cell_pos.distance_to(p_sample_position_map),
                    .idx = i
                };
            }
        }

        std::stable_sort(
            distances.begin(),
            distances.end(),
            [](const CellDistance &a, const CellDistance &b) {
                return a.distance < b.distance;
            });
        for (int i = 0; i < 9; i++) {
            r_result.closest_cell_centers[i] = distances[i].cell_center;
            r_result.closest_cell_positions[i] = distances[i].pos;
            r_result.closest_distances[i] = distances[i].distance;
        }
    }

protected:
    static void _bind_methods();

    static Ref<Voronoise> create_bind(uint32_t p_grid_size, uint32_t p_map_seed, Vector2i p_map_offset);

    float sample_bind(Vector2 p_sample_pos) const;
public:
    Voronoise(uint32_t p_map_seed) : map_seed(p_map_seed) {
    }
};

#endif // VORONOISE_H
