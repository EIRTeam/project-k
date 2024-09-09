#include "worldgen_height.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "scene/resources/curve.h"

float WorldgenHeight::get_height(const Vector2 &p_position) const {
    float f = settings->get_frequency();
    float t = settings->get_noise()->get_noise_2dv(p_position * f) * 0.5f + 0.5f; 

    const Ref<Curve> height_curve = settings->get_height_curve();
    const float height_multiplier = settings->get_height_multiplier();


    return height_curve->sample(t) * t * height_multiplier;
}

void WorldgenHeight::get_height_with_derivative(const Vector2 &p_position, float &r_height, Vector2 &r_derivative, float p_dir_eps) const {
    float sample_height = get_height(p_position);
    float h2 = get_height(p_position + Vector2(p_dir_eps, 0.0f));
    float h3 = get_height(p_position + Vector2(0.0f, p_dir_eps));

    r_height = sample_height;

    float slope_x = h2 - sample_height;
    float slope_z = h3 - sample_height;
    r_derivative = Vector2(slope_x, slope_z) / p_dir_eps;
}

void WorldgenHeight::get_height_with_normal(const Vector2 &p_position, Vector3 &r_normal, float &r_height) const {
    Vector2 deriv;
    float height;
    get_height_with_derivative(p_position, height, deriv);

    Vector3 normal = Vector3(-deriv.x, 1.0f, -deriv.y).normalized();
    r_normal = normal;
    r_height = height;
}

WorldgenHeight::WorldgenHeight() {
    fnl.instantiate();
    fnl->set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);
    fnl->set_fractal_type(FastNoiseLite::FRACTAL_NONE);
}

int WorldgenHeight::get_seed() const {
    return seed;
}

void WorldgenHeight::set_seed(int p_seed) {
    fnl->set_seed(p_seed);
}
_FORCE_INLINE_ float WorldgenHeightSettings::sample_height_curve(float p_t) const {
    if (!height_curve.is_valid()) {
        return p_t;
    }
    return height_curve->sample(p_t);
}
void WorldgenHeightSettings::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_frequency"), &WorldgenHeightSettings::get_frequency);
    ClassDB::bind_method(D_METHOD("set_frequency", "frequency"), &WorldgenHeightSettings::set_frequency);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frequency"), "set_frequency", "get_frequency");

    ClassDB::bind_method(D_METHOD("get_base_amplitude"), &WorldgenHeightSettings::get_base_amplitude);
    ClassDB::bind_method(D_METHOD("set_base_amplitude", "base_amplitude"), &WorldgenHeightSettings::set_base_amplitude);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_amplitude"), "set_base_amplitude", "get_base_amplitude");

    ClassDB::bind_method(D_METHOD("get_octaves"), &WorldgenHeightSettings::get_octaves);
    ClassDB::bind_method(D_METHOD("set_octaves", "octaves"), &WorldgenHeightSettings::set_octaves);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "octaves"), "set_octaves", "get_octaves");

    ClassDB::bind_method(D_METHOD("get_height_curve"), &WorldgenHeightSettings::get_height_curve);
    ClassDB::bind_method(D_METHOD("set_height_curve", "height_curve"), &WorldgenHeightSettings::set_height_curve);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "height_curve", PROPERTY_HINT_RESOURCE_TYPE, "Curve", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT), "set_height_curve", "get_height_curve");
    
    ClassDB::bind_method(D_METHOD("get_height_multiplier"), &WorldgenHeightSettings::get_height_multiplier);
    ClassDB::bind_method(D_METHOD("set_height_multiplier", "height_multiplier"), &WorldgenHeightSettings::set_height_multiplier);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height_multiplier"), "set_height_multiplier", "get_height_multiplier");
    
    ClassDB::bind_method(D_METHOD("get_noise"), &WorldgenHeightSettings::get_noise);
    ClassDB::bind_method(D_METHOD("set_noise", "noise"), &WorldgenHeightSettings::set_noise);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_noise", "get_noise");
}

float WorldgenHeightSettings::get_frequency() const {
    return frequency;
}

void WorldgenHeightSettings::set_frequency(float p_frequency) {
    frequency = p_frequency;
}

float WorldgenHeightSettings::get_base_amplitude() const {
    return base_amplitude;
}

void WorldgenHeightSettings::set_base_amplitude(float p_base_amplitude) {
    base_amplitude = p_base_amplitude;
}

Ref<Curve> WorldgenHeightSettings::get_height_curve() const {
    return height_curve;
}

void WorldgenHeightSettings::set_height_curve(Ref<Curve> p_height_curve) {
    height_curve = p_height_curve;
}

void WorldgenHeightSettings::set_height_multiplier(float p_height_multiplier) { height_multiplier = p_height_multiplier; }

Ref<FastNoiseLite> WorldgenHeightSettings::get_noise() const { return noise; }

void WorldgenHeightSettings::set_noise(Ref<FastNoiseLite> p_noise) { noise = p_noise; }
