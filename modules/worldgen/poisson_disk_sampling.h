#ifndef POISSON_DISK_SAMPLING_H
#define POISSON_DISK_SAMPLING_H

#include "core/math/random_pcg.h"
#include "core/math/rect2.h"
#include "core/math/rect2i.h"
#include "core/math/vector2.h"
#include "core/object/ref_counted.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"
#include "core/typedefs.h"
#include "core/variant/dictionary.h"
#include "core/variant/typed_array.h"
class PoissonDiskSampler {
public:
    struct PoissonSampleSettings {
        Vector2 sample_area = Vector2(512, 512);
        float element_radius = 64.0f;
        uint64_t seed = 0;
        int max_placement_attempts = 30;
    };
private:
    PoissonSampleSettings settings;
public:
    struct PoissonSampleCell {
        float angle_rads = 0.0f;
        Vector2 position;
    };
private:
    Rect2 real_grid_bounds;
    Vector2 grid_dimensions;
    float cell_size;
public:
    PoissonDiskSampler(const PoissonSampleSettings &p_settings);
    LocalVector<PoissonSampleCell> generate();
};

#endif // POISSON_DISK_SAMPLING_H
