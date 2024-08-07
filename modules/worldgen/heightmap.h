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
        int width = dimension;
        int height = dimension;
        Vector2 clamped_sample_point = p_point.clamp(Vector2(), Vector2(dimension-1, dimension-1));

        // Get the integer parts of the sample coordinates
        int x0 = (int)clamped_sample_point.x;
        int y0 = (int)clamped_sample_point.y;
        
        // Get the fractional parts of the sample coordinates
        float tx = clamped_sample_point.x - x0;
        float ty = clamped_sample_point.y - y0;

        // Ensure the coordinates for the surrounding points are within bounds
        int x1 = x0 + 1 < width ? x0 + 1 : x0;
        int y1 = y0 + 1 < height ? y0 + 1 : y0;

        // Retrieve the values at the four corners
        float v00 = data[y0 * width + x0];
        float v10 = data[y0 * width + x1];
        float v01 = data[y1 * width + x0];
        float v11 = data[y1 * width + x1];

        // Perform bilinear interpolation
        float i0 = Math::lerp(v00, v10, tx);
        float i1 = Math::lerp(v01, v11, tx);
        float value = Math::lerp(i0, i1, ty);

        if (value < -200) {
            print_line("COCK");
        }

        return value;
    }

    static void _bind_methods() {
        ClassDB::bind_static_method("WorldgenHeightmap", D_METHOD("create", "data", "dimension"), &WorldgenBilinearArray::create);
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
