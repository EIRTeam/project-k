#include "poisson_disk_sampling.h"

#include "core/math/math_defs.h"
#include "core/math/random_pcg.h"
#include "core/math/rect2.h"
#include "core/math/vector2i.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"
#include "core/typedefs.h"

LocalVector<PoissonDiskSampler::PoissonSampleCell> PoissonDiskSampler::generate() {
    RandomPCG pcg = RandomPCG(settings.seed);
    Vector<int> active_indices;
    LocalVector<PoissonSampleCell> inserted_cells;
    LocalVector<int> grid;

    grid.resize(grid_dimensions.x*grid_dimensions.y);
    for (uint32_t i = 0; i < grid.size(); i++) {
        grid[i] = -1;
    }

    {
        const Vector2 first_point_pos = Vector2(pcg.random(0.0f, real_grid_bounds.size.x), pcg.random(0.0f, real_grid_bounds.size.y));
        const Vector2i first_point_grid_pos = Vector2i(first_point_pos / cell_size).clamp(Vector2i(), grid_dimensions - Vector2i(1, 1));
        int first_point_idx = first_point_grid_pos.x + first_point_grid_pos.y * grid_dimensions.x;
        inserted_cells.push_back({
            .angle_rads = pcg.random(0.0f, (float)Math_TAU),
            .position = first_point_pos
        });
        active_indices.push_back(inserted_cells.size()-1);
        grid[first_point_idx] = inserted_cells.size()-1;
    }

    while (active_indices.size() > 0) {
        bool inserted = false;
        const int origin_index = active_indices[pcg.random(0, active_indices.size()-1)];
        const Vector2 origin_position = inserted_cells[origin_index].position;
        for (int k = 0; k < 20; k++) {
            const float distance = pcg.random(0.0, settings.element_radius*2.0);
            const Vector2 new_point_position = origin_position + Vector2::from_angle(pcg.random(0.0, Math_TAU)) * distance;
            if (!real_grid_bounds.has_point(new_point_position)) {
                continue;
            }

            const Vector2i new_point_grid_position = Vector2i(new_point_position / cell_size).clamp(Vector2i(), grid_dimensions - Vector2i(1, 1));

            bool can_insert = true;

            for (int x = MAX(new_point_grid_position.x-2, 0); x <= MIN(grid_dimensions.x-1, new_point_grid_position.x+2); x++) {
                for (int y = MAX(new_point_grid_position.y-2, 0); y <= MIN(grid_dimensions.y-1, new_point_grid_position.y+2); y++) {
                    int test_idx = x + y * grid_dimensions.x;
                    if (grid[test_idx] != -1 && inserted_cells[grid[test_idx]].position.distance_to(new_point_position) < settings.element_radius) {
                        can_insert = false;
                        break;
                    }
                }
            }

            if (can_insert) {
                int insertion_idx = new_point_grid_position.x + new_point_grid_position.y * grid_dimensions.x;
                inserted_cells.push_back({
                    .position = new_point_position
                });
                grid[insertion_idx] = inserted_cells.size()-1;
                active_indices.push_back(inserted_cells.size()-1);
                inserted = true;
                break;
            }
        }

        if (!inserted) {
            active_indices.erase(origin_index);
        }
    }
    return inserted_cells;
}

PoissonDiskSampler::PoissonDiskSampler(const PoissonSampleSettings &p_settings) {
    settings = p_settings;
    cell_size = settings.element_radius / Math_SQRT2;
    // Might not fit exactly, we should undershoot
    grid_dimensions = Vector2i(settings.sample_area / cell_size);
    real_grid_bounds = Rect2(Vector2(), cell_size * grid_dimensions);
}
