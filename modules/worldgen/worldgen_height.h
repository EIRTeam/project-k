#ifndef WORLDGEN_HEIGHT_H
#define WORLDGEN_HEIGHT_H

#include "core/typedefs.h"
#include "modules/noise/fastnoise_lite.h"

class Curve;
class WorldgenHeightSettings : public Resource {
    GDCLASS(WorldgenHeightSettings, Resource);
    Ref<Curve> height_curve;
    Ref<FastNoiseLite> noise;
    float frequency = 0.15f;
    float base_amplitude = 1.0f;
    int octaves = 8;
    float height_multiplier = 1.0f;

    _FORCE_INLINE_ float sample_height_curve(float p_t) const;

protected:
    static void _bind_methods();
public:

    float get_frequency() const;
    void set_frequency(float p_frequency);

    float get_base_amplitude() const;
    void set_base_amplitude(float p_base_amplitude);

    Ref<Curve> get_height_curve() const;
    void set_height_curve(Ref<Curve> p_height_curve);

    int get_octaves() const { return octaves; }
    void set_octaves(int p_octaves) { octaves = p_octaves; }

    float get_height_multiplier() const { return height_multiplier; }
    void set_height_multiplier(float p_height_multiplier);

    Ref<FastNoiseLite> get_noise() const;
    void set_noise(Ref<FastNoiseLite> p_noise);
};

class WorldgenHeight : public RefCounted {
    Ref<FastNoiseLite> fnl;
    Ref<WorldgenHeightSettings> settings;
    int seed;
public:
    float get_height(const Vector2 &p_position) const;
    void get_height_with_derivative(const Vector2 &p_position, float &r_height, Vector2 &r_derivative, float p_dir_eps = 0.01f) const;
    void get_height_with_normal(const Vector2 &p_position, Vector3 &r_normal, float &r_height) const;

    int get_seed() const;
    void set_seed(int p_seed);

    void set_settings(Ref<WorldgenHeightSettings> p_settings) {
        settings = p_settings;
    }

    WorldgenHeight();
};

#endif // WORLDGEN_HEIGHT_H
