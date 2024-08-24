#include "vehicle_engine.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "vehicle_constants.h"

float VehicleEngine::advance(float p_delta, float p_throttle, float p_clutch_reaction_torque) {
    time += p_delta;
    float engine_torque = settings->get_torque_at_rpm(rpm, p_throttle);
    
    const float limiter_cycle = 1.0f / settings->get_limiter_hz();

    if (time - last_limiter_trigger < limiter_cycle) {
        engine_torque = settings->get_torque_at_rpm(rpm, 0.0f);
    }

    engine_torque += p_clutch_reaction_torque;

    rpm += VehicleConstants::AV_TO_RPM * p_delta * engine_torque / settings->get_inertia();

    rpm = MAX(850, rpm);

    if (rpm >= settings->get_rpm_limit()) {
        last_limiter_trigger = time;
    }

    return engine_torque;
}

void VehicleEngine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_angular_velocity"), &VehicleEngine::get_angular_velocity);
    ClassDB::bind_method(D_METHOD("advance", "delta", "throttle", "clutch_reaction_torque"), &VehicleEngine::advance);
    ClassDB::bind_method(D_METHOD("get_rpm"), &VehicleEngine::get_rpm);
    
    ClassDB::bind_method(D_METHOD("get_settings"), &VehicleEngine::get_settings);
    ClassDB::bind_method(D_METHOD("set_settings", "settings"), &VehicleEngine::set_settings);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "settings", PROPERTY_HINT_RESOURCE_TYPE, "VehicleEngineSettings", PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT | PROPERTY_USAGE_DEFAULT), "set_settings", "get_settings");
}

float VehicleEngine::get_angular_velocity() const {
    return rpm / VehicleConstants::AV_TO_RPM;
}

float VehicleEngine::get_rpm() const {
    return rpm;
}

Ref<VehicleEngineSettings> VehicleEngine::get_settings() const { return settings; }

void VehicleEngine::set_settings(Ref<VehicleEngineSettings> p_settings) { settings = p_settings; }
