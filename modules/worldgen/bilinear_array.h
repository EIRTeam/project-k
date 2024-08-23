#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/string/print_string.h"
#include "core/templates/vector.h"

class WorldgenBilinearArray : public RefCounted {
    GDCLASS(WorldgenBilinearArray, RefCounted);
    LocalVector<float> data;
    int dimension;
public:
    float sample(const Vector2 &p_point) {
        Vector2 sample_point = p_point.round();
        Vector2i sample_2 = Vector2i(sample_point.x, sample_point.y).clampi(0, dimension-1);
        Vector2i sample_1 = (sample_2 - Vector2i(1, 1)).clampi(0, dimension-1);
        Vector2 sample_weights; 
        sample_weights.x = Math::inverse_lerp(sample_1.x + 0.5f, sample_2.x + 0.5f, p_point.x);
        sample_weights.y = Math::inverse_lerp(sample_1.y + 0.5f, sample_2.y + 0.5f, p_point.y);

        sample_weights = sample_weights.clampf(0.0, 1.0f);

        float value_top_x0 = data[sample_1.x + sample_1.y * dimension];
        float value_top_x1 = data[sample_2.x + sample_1.y * dimension];
        float value_bottom_x0 = data[sample_1.x + sample_2.y * dimension];
        float value_bottom_x1 = data[sample_2.x + sample_2.y * dimension];

        float top_x_interp = Math::lerp(value_top_x0, value_top_x1, sample_weights.x);
        float bottom_x_interp = Math::lerp(value_bottom_x0, value_bottom_x1, sample_weights.x);

        return Math::lerp(top_x_interp, bottom_x_interp, sample_weights.y);
    }

    static void _bind_methods() {
        ClassDB::bind_static_method("WorldgenBilinearArray", D_METHOD("create", "data", "dimension"), &WorldgenBilinearArray::create);
        ClassDB::bind_method(D_METHOD("sample", "point"), &WorldgenBilinearArray::sample);
    }
public:
    void set_pixel(const Vector2i &p_pixel, float p_height) {
        uint32_t idx = p_pixel.x + p_pixel.y * dimension;
        ERR_FAIL_INDEX(idx, data.size());
        data.ptr()[idx] = p_height;
    }

    float get_pixel(const Vector2i &p_pixel) const {
        uint32_t idx = p_pixel.x + p_pixel.y * dimension;
        ERR_FAIL_INDEX_V(idx, data.size(), 0.0f);
        return data[idx];
    }

    Vector<float> get_data() const {
        return data;
    };

    static Ref<WorldgenBilinearArray> create(Vector<float> p_data, int p_dimension) {
        Ref<WorldgenBilinearArray> heightmap;
        heightmap.instantiate();
        ERR_FAIL_COND_V((p_dimension*p_dimension) != p_data.size(), Ref<WorldgenBilinearArray>());
        
        heightmap->data = p_data;
        heightmap->dimension = p_dimension;

        return heightmap;
    }

    int get_dimension() const {
        return dimension;
    }

    static Ref<WorldgenBilinearArray> create_xy(int p_dimension) {
        Ref<WorldgenBilinearArray> heightmap;
        heightmap.instantiate();
        
        heightmap->data.resize(p_dimension*p_dimension);
        heightmap->dimension = p_dimension;

        return heightmap;
    }
};

#endif // HEIGHTMAP_H
