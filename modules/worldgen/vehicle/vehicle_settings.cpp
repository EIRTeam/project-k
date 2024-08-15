#include "vehicle_settings.h"
#include "core/object/class_db.h"
#include "core/object/object.h"

void VehicleEngineSettings::_update_torque_curve() {
  if (!torque_curve_baked.is_valid()) {
    torque_curve_baked.instantiate();
  }
  torque_curve_baked->clear_points();

  if (torque_curve.size() == 0) {
    return;
  }

  float max_rpm = torque_curve[0].x;
  for (int i = 1; i < torque_curve.size(); i++) {
    max_rpm = MAX(max_rpm, torque_curve[i].x);
  }

  for (int i = 0; i < torque_curve.size(); i++) {
    float rpm = torque_curve[i].x;
    float torque = torque_curve[i].y;
    torque_curve_baked->add_point(Vector2(rpm / max_rpm, torque));
  }
  torque_curve_max_rpm = max_rpm;
}

void VehicleEngineSettings::_bind_methods() {
  ClassDB::bind_method(D_METHOD("set_rpm_limit", "rpm_limit"),
                       &VehicleEngineSettings::set_rpm_limit);
  ClassDB::bind_method(D_METHOD("get_rpm_limit"),
                       &VehicleEngineSettings::get_rpm_limit);
  ADD_PROPERTY(PropertyInfo(Variant::INT, "rpm_limit"), "set_rpm_limit",
               "get_rpm_limit");

  ClassDB::bind_method(D_METHOD("set_limiter_hz", "limiter_hz"),
                       &VehicleEngineSettings::set_limiter_hz);
  ClassDB::bind_method(D_METHOD("get_limiter_hz"),
                       &VehicleEngineSettings::get_limiter_hz);
  ADD_PROPERTY(
      PropertyInfo(Variant::INT, "limiter_hz", PROPERTY_HINT_RANGE, "1,6000,1"),
      "set_limiter_hz", "get_limiter_hz");

  ClassDB::bind_method(D_METHOD("set_torque_curve", "torque_curve"),
                       &VehicleEngineSettings::set_torque_curve);
  ClassDB::bind_method(D_METHOD("get_torque_curve"),
                       &VehicleEngineSettings::get_torque_curve);
  ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "torque_curve"),
               "set_torque_curve", "get_torque_curve");

  ClassDB::bind_method(D_METHOD("set_throttle_map", "throttle_map"),
                       &VehicleEngineSettings::set_throttle_map);
  ClassDB::bind_method(D_METHOD("get_throttle_map"),
                       &VehicleEngineSettings::get_throttle_map);
  ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "throttle_map"),
               "set_throttle_map", "get_throttle_map");

  ClassDB::bind_method(D_METHOD("set_coast_ref_rpm", "coast_ref_rpm"),
                       &VehicleEngineSettings::set_coast_ref_rpm);
  ClassDB::bind_method(D_METHOD("get_coast_ref_rpm"),
                       &VehicleEngineSettings::get_coast_ref_rpm);
  ADD_PROPERTY(PropertyInfo(Variant::INT, "coast_ref_rpm", PROPERTY_HINT_RANGE,
                            "0,10000,1"),
               "set_coast_ref_rpm", "get_coast_ref_rpm");

  ClassDB::bind_method(D_METHOD("set_coast_ref_torque", "coast_ref_torque"),
                       &VehicleEngineSettings::set_coast_ref_torque);
  ClassDB::bind_method(D_METHOD("get_coast_ref_torque"),
                       &VehicleEngineSettings::get_coast_ref_torque);
  ADD_PROPERTY(PropertyInfo(Variant::INT, "coast_ref_torque",
                            PROPERTY_HINT_RANGE, "0,10000,1"),
               "set_coast_ref_torque", "get_coast_ref_torque");

  ClassDB::bind_method(
      D_METHOD("set_coast_curve_non_linearity", "coast_curve_non_linearity"),
      &VehicleEngineSettings::set_coast_curve_non_linearity);
  ClassDB::bind_method(D_METHOD("get_coast_curve_non_linearity"),
                       &VehicleEngineSettings::get_coast_curve_non_linearity);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "coast_curve_non_linearity",
                            PROPERTY_HINT_RANGE, "0,1,0.01"),
               "set_coast_curve_non_linearity",
               "get_coast_curve_non_linearity");

  ClassDB::bind_method(D_METHOD("set_inertia", "inertia"),
                       &VehicleEngineSettings::set_inertia);
  ClassDB::bind_method(D_METHOD("get_inertia"),
                       &VehicleEngineSettings::get_inertia);
  ADD_PROPERTY(
      PropertyInfo(Variant::FLOAT, "inertia", PROPERTY_HINT_RANGE, "0,1,0.01"),
      "set_inertia", "get_inertia");

  ClassDB::bind_method(D_METHOD("set_idle_rpm", "idle_rpm"),
                       &VehicleEngineSettings::set_idle_rpm);
  ClassDB::bind_method(D_METHOD("get_idle_rpm"),
                       &VehicleEngineSettings::get_idle_rpm);
  ADD_PROPERTY(PropertyInfo(Variant::INT, "idle_rpm"), "set_idle_rpm",
               "get_idle_rpm");

  ClassDB::bind_method(D_METHOD("get_torque_at_rpm", "rpm", "throttle"),
                       &VehicleEngineSettings::get_torque_at_rpm);
}

float VehicleEngineSettings::get_coast_torque_at_rpm(float p_rpm) const {
  float p = Math::pow(p_rpm / coast_ref_rpm, 1 + coast_curve_non_linearity);
  return coast_ref_torque * p;
}

int VehicleEngineSettings::get_rpm_limit() const { return rpm_limit; }

void VehicleEngineSettings::set_rpm_limit(int p_rpm_limit) {
  torque_curve_dirty = true;
  rpm_limit = p_rpm_limit;
}

PackedVector2Array VehicleEngineSettings::get_torque_curve() const {
  return torque_curve;
}

void VehicleEngineSettings::set_torque_curve(
    PackedVector2Array p_torque_curve) {
  torque_curve_dirty = true;
  torque_curve = p_torque_curve;
}

PackedVector2Array VehicleEngineSettings::get_throttle_map() const {
  return throttle_map;
}

void VehicleEngineSettings::set_throttle_map(
    PackedVector2Array p_throttle_map) {
  throttle_map = p_throttle_map;

  if (!throttle_map_baked.is_valid()) {
    throttle_map_baked.instantiate();
  }
  throttle_map_baked->clear_points();
  for (int i = 0; i < throttle_map.size(); i++) {
    throttle_map_baked->add_point(throttle_map[i]);
  }
}

int VehicleEngineSettings::get_coast_ref_rpm() const { return coast_ref_rpm; }

void VehicleEngineSettings::set_coast_ref_rpm(int p_coast_ref_rpm) {
  coast_curve_dirty = true;
  coast_ref_rpm = p_coast_ref_rpm;
}

int VehicleEngineSettings::get_coast_ref_torque() const {
  return coast_ref_torque;
}

void VehicleEngineSettings::set_coast_ref_torque(int p_coast_ref_torque) {
  coast_curve_dirty = true;
  coast_ref_torque = p_coast_ref_torque;
}

float VehicleEngineSettings::get_coast_curve_non_linearity() const {
  return coast_curve_non_linearity;
}

void VehicleEngineSettings::set_coast_curve_non_linearity(
    float p_coast_curve_non_linearity) {
  coast_curve_dirty = true;
  coast_curve_non_linearity = p_coast_curve_non_linearity;
}

float VehicleEngineSettings::get_inertia() const { return inertia; }

void VehicleEngineSettings::set_inertia(float p_inertia) {
  inertia = p_inertia;
}

int VehicleEngineSettings::get_idle_rpm() const { return idle_rpm; }

void VehicleEngineSettings::set_idle_rpm(int p_idle_rpm) {
  idle_rpm = p_idle_rpm;
}

float VehicleEngineSettings::sample_throttle_map(float p_throttle) const {
  return throttle_map_baked->sample_baked(p_throttle);
}

float VehicleEngineSettings::get_on_throttle_torque_at_rpm(float p_rpm) const {
  if (torque_curve_dirty) {
    const_cast<VehicleEngineSettings *>(this)->_update_torque_curve();
  }

  return torque_curve_baked->sample(p_rpm / torque_curve_max_rpm);
}

float VehicleEngineSettings::get_torque_at_rpm(float p_rpm,
                                               float p_throttle) const {
  float torque = get_on_throttle_torque_at_rpm(p_rpm);
  float coast_torque = get_coast_torque_at_rpm(p_rpm);
  return Math::lerp(-coast_torque, torque, sample_throttle_map(p_throttle));
}

int VehicleEngineSettings::get_limiter_hz() const { return limiter_hz; }

void VehicleEngineSettings::set_limiter_hz(int p_limiter_hz) {
  limiter_hz = p_limiter_hz;
}

void VehicleDifferentialSettings::_bind_methods() {
  BIND_ENUM_CONSTANT(OPEN);
  BIND_ENUM_CONSTANT(CLOSED);
  BIND_ENUM_CONSTANT(LIMITED_SLIP);

  ClassDB::bind_method(D_METHOD("set_type", "type"),
                       &VehicleDifferentialSettings::set_type);
  ClassDB::bind_method(D_METHOD("get_type"),
                       &VehicleDifferentialSettings::get_type);
  ADD_PROPERTY(PropertyInfo(Variant::INT, "type", PROPERTY_HINT_ENUM,
                            "Limited slip,Open,Closed"),
               "set_type", "get_type");

  ClassDB::bind_method(D_METHOD("set_coast_ratio", "coast_ratio"),
                       &VehicleDifferentialSettings::set_coast_ratio);
  ClassDB::bind_method(D_METHOD("get_coast_ratio"),
                       &VehicleDifferentialSettings::get_coast_ratio);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "coast_ratio", PROPERTY_HINT_RANGE,
                            "0,10,0.01"),
               "set_coast_ratio", "get_coast_ratio");

  ClassDB::bind_method(D_METHOD("set_power_ratio", "power_ratio"),
                       &VehicleDifferentialSettings::set_power_ratio);
  ClassDB::bind_method(D_METHOD("get_power_ratio"),
                       &VehicleDifferentialSettings::get_power_ratio);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "power_ratio", PROPERTY_HINT_RANGE,
                            "0,10,0.01"),
               "set_power_ratio", "get_power_ratio");

  ClassDB::bind_method(D_METHOD("set_preload", "preload"),
                       &VehicleDifferentialSettings::set_preload);
  ClassDB::bind_method(D_METHOD("get_preload"),
                       &VehicleDifferentialSettings::get_preload);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "preload"), "set_preload",
               "get_preload");
}

void VehicleCurveModelTireSettings::_bind_methods() {
  ClassDB::bind_method(D_METHOD("get_tire_stiffness"),
                       &VehicleCurveModelTireSettings::get_tire_stiffness);
  ClassDB::bind_method(D_METHOD("set_tire_stiffness", "tire_stiffness"),
                       &VehicleCurveModelTireSettings::set_tire_stiffness);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tire_stiffness"),
               "set_tire_stiffness", "get_tire_stiffness");

  ClassDB::bind_method(D_METHOD("get_buildup"),
                       &VehicleCurveModelTireSettings::get_buildup);
  ClassDB::bind_method(D_METHOD("set_buildup", "buildup"),
                       &VehicleCurveModelTireSettings::set_buildup);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "buildup",
                            PROPERTY_HINT_RESOURCE_TYPE, "Curve",
                            PROPERTY_USAGE_DEFAULT |
                                PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT),
               "set_buildup", "get_buildup");

  ClassDB::bind_method(D_METHOD("get_falloff"),
                       &VehicleCurveModelTireSettings::get_falloff);
  ClassDB::bind_method(D_METHOD("set_falloff", "falloff"),
                       &VehicleCurveModelTireSettings::set_falloff);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "falloff",
                            PROPERTY_HINT_RESOURCE_TYPE, "Curve",
                            PROPERTY_USAGE_DEFAULT |
                                PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT),
               "set_falloff", "get_falloff");

  ClassDB::bind_method(
      D_METHOD("update_forces", "slip_ratio", "normal_load", "surface_mu"),
      &VehicleCurveModelTireSettings::update_forces);
}

Vector3
VehicleCurveModelTireSettings::update_forces(const Vector2 p_slip_ratio,
                                             const float p_normal_load,
                                             const float p_surface_mu) const {
  // TODO: Add wear/temp here
  const float wear_mu = 1.0f;
  const float temp_mu = 1.0f;

  const float load_sensitivity = calculate_load_sensitivity(p_normal_load);
  float tire_mu = p_surface_mu * wear_mu * temp_mu * load_sensitivity;

  float load_factor = p_normal_load / get_tire_rated_load();
  float peak_sa_deg =
      Math::lerp(MAX_PEAK_SA_START_DEG, MIN_PEAK_SA_START_DEG, tire_stiffness);
  float delta_sa_deg =
      Math::lerp(MAX_DELTA_SA_DEG, MIN_DELTA_SA_DEG, tire_stiffness);

  float sa0 = peak_sa_deg + 0.5 * delta_sa_deg;
  float sa1 = peak_sa_deg - 0.5 * delta_sa_deg;
  float peak_sa = Math::deg_to_rad(Math::lerp(sa1, sa0, load_factor));

  float peak_sr_ratio = 0.65f;
  float peak_sr = peak_sa * peak_sr_ratio;

  float normalised_sr = p_slip_ratio.y / peak_sr;
  float normalised_sa = p_slip_ratio.x / peak_sa;
  float resultant_slip =
      Math::sqrt(normalised_sr * normalised_sr + normalised_sa * normalised_sa);

  float sr_modified = resultant_slip * peak_sr;
  float sa_modified = resultant_slip * peak_sa;

  Vector3 tire_forces;

  if (Math::abs(p_slip_ratio.x) < peak_sa) {
    tire_forces.x =
        buildup->sample_baked(resultant_slip) * SIGN(p_slip_ratio.x);
  } else {
    tire_forces.x =
        falloff->sample_baked(sa_modified - peak_sa) * SIGN(p_slip_ratio.x);
  }
  if (Math::abs(p_slip_ratio.y) < peak_sr) {
    tire_forces.y =
        buildup->sample_baked(resultant_slip) * SIGN(p_slip_ratio.y);
  } else {
    tire_forces.y =
        falloff->sample_baked(sr_modified - peak_sr) * SIGN(p_slip_ratio.y);
  }

  tire_forces *= tire_mu * p_normal_load;

  if (resultant_slip != 0.0f) {
    tire_forces.x *= Math::abs(normalised_sa / resultant_slip);
    tire_forces.y *= Math::abs(normalised_sr / resultant_slip);
  }

  return tire_forces;
}

Ref<Curve> VehicleCurveModelTireSettings::get_falloff() const {
  return falloff;
}

void VehicleCurveModelTireSettings::set_falloff(const Ref<Curve> &p_falloff) {
  falloff = p_falloff;
}

Ref<Curve> VehicleCurveModelTireSettings::get_buildup() const {
  return buildup;
}

void VehicleCurveModelTireSettings::set_buildup(const Ref<Curve> &p_buildup) {
  buildup = p_buildup;
}

void VehicleSettings::_bind_methods() {
  ClassDB::bind_method(D_METHOD("get_mass"), &VehicleSettings::get_mass);
  ClassDB::bind_method(D_METHOD("set_mass", "mass"),
                       &VehicleSettings::set_mass);
  ADD_PROPERTY(
      PropertyInfo(Variant::INT, "mass", PROPERTY_HINT_RANGE, "0, 3500, 1"),
      "set_mass", "get_mass");

  ClassDB::bind_method(D_METHOD("get_wheel_fl"),
                       &VehicleSettings::get_wheel_fl);
  ClassDB::bind_method(D_METHOD("set_wheel_fl", "wheel_fl"),
                       &VehicleSettings::set_wheel_fl);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "wheel_fl",
                            PROPERTY_HINT_RESOURCE_TYPE,
                            "VehicleSuspensionCornerSettings",
                            PROPERTY_USAGE_DEFAULT |
                                PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT),
               "set_wheel_fl", "get_wheel_fl");

  ClassDB::bind_method(D_METHOD("get_wheel_fr"),
                       &VehicleSettings::get_wheel_fr);
  ClassDB::bind_method(D_METHOD("set_wheel_fr", "wheel_fr"),
                       &VehicleSettings::set_wheel_fr);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "wheel_fr",
                            PROPERTY_HINT_RESOURCE_TYPE,
                            "VehicleSuspensionCornerSettings",
                            PROPERTY_USAGE_DEFAULT |
                                PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT),
               "set_wheel_fr", "get_wheel_fr");

  ClassDB::bind_method(D_METHOD("get_wheel_rl"),
                       &VehicleSettings::get_wheel_rl);
  ClassDB::bind_method(D_METHOD("set_wheel_rl", "wheel_rl"),
                       &VehicleSettings::set_wheel_rl);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "wheel_rl",
                            PROPERTY_HINT_RESOURCE_TYPE,
                            "VehicleSuspensionCornerSettings",
                            PROPERTY_USAGE_DEFAULT |
                                PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT),
               "set_wheel_rl", "get_wheel_rl");

  ClassDB::bind_method(D_METHOD("get_wheel_rr"),
                       &VehicleSettings::get_wheel_rr);
  ClassDB::bind_method(D_METHOD("set_wheel_rr", "wheel_rr"),
                       &VehicleSettings::set_wheel_rr);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "wheel_rr",
                            PROPERTY_HINT_RESOURCE_TYPE,
                            "VehicleSuspensionCornerSettings",
                            PROPERTY_USAGE_DEFAULT |
                                PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT),
               "set_wheel_rr", "get_wheel_rr");

  ClassDB::bind_method(D_METHOD("get_engine_settings"),
                       &VehicleSettings::get_engine_settings);
  ClassDB::bind_method(D_METHOD("set_engine_settings", "engine_settings"),
                       &VehicleSettings::set_engine_settings);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "engine_settings",
                            PROPERTY_HINT_RESOURCE_TYPE,
                            "VehicleEngineSettings",
                            PROPERTY_USAGE_DEFAULT |
                                PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT),
               "set_engine_settings", "get_engine_settings");

  ClassDB::bind_method(D_METHOD("get_drivetrain_settings"),
                       &VehicleSettings::get_drivetrain_settings);
  ClassDB::bind_method(
      D_METHOD("set_drivetrain_settings", "drivetrain_settings"),
      &VehicleSettings::set_drivetrain_settings);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "drivetrain_settings",
                            PROPERTY_HINT_RESOURCE_TYPE,
                            "VehicleDrivetrainSettings",
                            PROPERTY_USAGE_DEFAULT |
                                PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT),
               "set_drivetrain_settings", "get_drivetrain_settings");

  ClassDB::bind_method(D_METHOD("get_brake_front_share"),
                       &VehicleSettings::get_brake_front_share);
  ClassDB::bind_method(D_METHOD("set_brake_front_share", "brake_front_share"),
                       &VehicleSettings::set_brake_front_share);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "brake_front_share",
                            PROPERTY_HINT_RANGE, "0,1,0.01"),
               "set_brake_front_share", "get_brake_front_share");

  ClassDB::bind_method(D_METHOD("get_ackermann"),
                       &VehicleSettings::get_ackermann);
  ClassDB::bind_method(D_METHOD("set_ackermann", "ackermann"),
                       &VehicleSettings::set_ackermann);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ackermann", PROPERTY_HINT_RANGE,
                            "0,1,0.01"),
               "set_ackermann", "get_ackermann");

  ClassDB::bind_method(D_METHOD("get_max_steer_angle"),
                       &VehicleSettings::get_max_steer_angle);
  ClassDB::bind_method(D_METHOD("set_max_steer_angle", "max_steer_angle"),
                       &VehicleSettings::set_max_steer_angle);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_steer_angle"),
               "set_max_steer_angle", "get_max_steer_angle");

  ClassDB::bind_method(D_METHOD("get_max_brake_torque"),
                       &VehicleSettings::get_max_brake_torque);
  ClassDB::bind_method(D_METHOD("set_max_brake_torque", "max_brake_torque"),
                       &VehicleSettings::set_max_brake_torque);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_brake_torque"),
               "set_max_brake_torque", "get_max_brake_torque");

  ClassDB::bind_method(D_METHOD("get_max_handbrake_torque"),
                       &VehicleSettings::get_max_handbrake_torque);
  ClassDB::bind_method(
      D_METHOD("set_max_handbrake_torque", "max_handbrake_torque"),
      &VehicleSettings::set_max_handbrake_torque);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_handbrake_torque"),
               "set_max_handbrake_torque", "get_max_handbrake_torque");
}

Ref<VehicleSuspensionCornerSettings> VehicleSettings::get_wheel_fl() const {
  return wheel_fl;
}

void VehicleSettings::set_wheel_fl(
    const Ref<VehicleSuspensionCornerSettings> &p_wheel_fl) {
  wheel_fl = p_wheel_fl;
}

Ref<VehicleSuspensionCornerSettings> VehicleSettings::get_wheel_fr() const {
  return wheel_fr;
}

void VehicleSettings::set_wheel_fr(
    const Ref<VehicleSuspensionCornerSettings> &p_wheel_fr) {
  wheel_fr = p_wheel_fr;
}

Ref<VehicleSuspensionCornerSettings> VehicleSettings::get_wheel_rl() const {
  return wheel_rl;
}

void VehicleSettings::set_wheel_rl(
    const Ref<VehicleSuspensionCornerSettings> &p_wheel_rl) {
  wheel_rl = p_wheel_rl;
}

Ref<VehicleSuspensionCornerSettings> VehicleSettings::get_wheel_rr() const {
  return wheel_rr;
}

void VehicleSettings::set_wheel_rr(
    const Ref<VehicleSuspensionCornerSettings> &p_wheel_rr) {
  wheel_rr = p_wheel_rr;
}

int VehicleSettings::get_mass() const { return mass; }

void VehicleSettings::set_mass(int p_mass) { mass = p_mass; }

Ref<VehicleEngineSettings> VehicleSettings::get_engine_settings() const {
  return engine_settings;
}

void VehicleSettings::set_engine_settings(
    const Ref<VehicleEngineSettings> &p_engine_settings) {
  engine_settings = p_engine_settings;
}

float VehicleSettings::get_brake_front_share() const {
  return brake_front_share;
}

void VehicleSettings::set_brake_front_share(float p_brake_front_share) {
  brake_front_share = p_brake_front_share;
}

float VehicleSettings::get_max_brake_torque() const { return max_brake_torque; }

void VehicleSettings::set_max_brake_torque(float p_max_brake_torque) {
  max_brake_torque = p_max_brake_torque;
}

float VehicleSettings::get_max_handbrake_torque() const {
  return max_handbrake_torque;
}

void VehicleSettings::set_max_handbrake_torque(float p_max_handbrake_torque) {
  max_handbrake_torque = p_max_handbrake_torque;
}

float VehicleSettings::get_steering_speed() const { return steering_speed; }

void VehicleSettings::set_steering_speed(float p_steering_speed) {
  steering_speed = p_steering_speed;
}

float VehicleSettings::get_ackermann() const { return ackermann; }

void VehicleSettings::set_ackermann(float p_ackermann) {
  ackermann = p_ackermann;
}

float VehicleSettings::get_max_steer_angle() const { return max_steer_angle; }

void VehicleSettings::set_max_steer_angle(float p_max_steer_angle) {
  max_steer_angle = p_max_steer_angle;
}

Ref<VehicleDrivetrainSettings>
VehicleSettings::get_drivetrain_settings() const {
  return drivetrain_settings;
}

void VehicleSettings::set_drivetrain_settings(
    const Ref<VehicleDrivetrainSettings> &p_drivetrain_settings) {
  drivetrain_settings = p_drivetrain_settings;
}

void VehicleSuspensionCornerSettings::_bind_methods() {
  ClassDB::bind_method(D_METHOD("set_spring_travel", "spring_travel"),
                       &VehicleSuspensionCornerSettings::set_spring_travel);
  ClassDB::bind_method(D_METHOD("get_spring_travel"),
                       &VehicleSuspensionCornerSettings::get_spring_travel);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spring_travel",
                            PROPERTY_HINT_RANGE, "0.01,2,0.001"),
               "set_spring_travel", "get_spring_travel");

  ClassDB::bind_method(D_METHOD("set_tire_settings", "tire_settings"),
                       &VehicleSuspensionCornerSettings::set_tire_settings);
  ClassDB::bind_method(D_METHOD("get_tire_settings"),
                       &VehicleSuspensionCornerSettings::get_tire_settings);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "tire_settings",
                            PROPERTY_HINT_RESOURCE_TYPE, "VehicleTireSettings",
                            PROPERTY_USAGE_DEFAULT |
                                PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT),
               "set_tire_settings", "get_tire_settings");

  ClassDB::bind_method(D_METHOD("set_bump", "bump"),
                       &VehicleSuspensionCornerSettings::set_bump);
  ClassDB::bind_method(D_METHOD("get_bump"),
                       &VehicleSuspensionCornerSettings::get_bump);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bump", PROPERTY_HINT_RANGE,
                            "0.01,20,0.001"),
               "set_bump", "get_bump");

  ClassDB::bind_method(D_METHOD("set_rebound", "rebound"),
                       &VehicleSuspensionCornerSettings::set_rebound);
  ClassDB::bind_method(D_METHOD("get_rebound"),
                       &VehicleSuspensionCornerSettings::get_rebound);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rebound", PROPERTY_HINT_RANGE,
                            "0.01,20,0.001"),
               "set_rebound", "get_rebound");

  ClassDB::bind_method(D_METHOD("set_anti_roll", "anti_roll"),
                       &VehicleSuspensionCornerSettings::set_anti_roll);
  ClassDB::bind_method(D_METHOD("get_anti_roll"),
                       &VehicleSuspensionCornerSettings::get_anti_roll);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "anti_roll", PROPERTY_HINT_RANGE,
                            "0.01,20,0.001"),
               "set_anti_roll", "get_anti_roll");

  ClassDB::bind_method(D_METHOD("set_spring_rate", "anti_roll"),
                       &VehicleSuspensionCornerSettings::set_spring_rate);
  ClassDB::bind_method(D_METHOD("get_spring_rate"),
                       &VehicleSuspensionCornerSettings::get_spring_rate);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spring_rate"), "set_spring_rate",
               "get_spring_rate");

  ClassDB::bind_method(D_METHOD("set_wheel_mass", "wheel_mass"),
                       &VehicleSuspensionCornerSettings::set_wheel_mass);
  ClassDB::bind_method(D_METHOD("get_wheel_mass"),
                       &VehicleSuspensionCornerSettings::get_wheel_mass);
  ADD_PROPERTY(
      PropertyInfo(Variant::INT, "wheel_mass", PROPERTY_HINT_RANGE, "1,30,1"),
      "set_wheel_mass", "get_wheel_mass");
}

float VehicleSuspensionCornerSettings::get_spring_travel() const {
  return spring_travel;
}

void VehicleSuspensionCornerSettings::set_spring_travel(float p_spring_travel) {
  spring_travel = p_spring_travel;
}

Ref<VehicleTireSettings>
VehicleSuspensionCornerSettings::get_tire_settings() const {
  return tire_settings;
}

void VehicleSuspensionCornerSettings::set_tire_settings(
    const Ref<VehicleTireSettings> &p_tire_settings) {
  tire_settings = p_tire_settings;
}

float VehicleSuspensionCornerSettings::get_spring_rate() const {
  return spring_rate;
}

void VehicleSuspensionCornerSettings::set_spring_rate(float p_spring_rate) {
  spring_rate = p_spring_rate;
}

float VehicleSuspensionCornerSettings::get_bump() const { return bump; }

void VehicleSuspensionCornerSettings::set_bump(float p_bump) { bump = p_bump; }

float VehicleSuspensionCornerSettings::get_rebound() const { return rebound; }

void VehicleSuspensionCornerSettings::set_rebound(float p_rebound) {
  rebound = p_rebound;
}

float VehicleSuspensionCornerSettings::get_anti_roll() const {
  return anti_roll;
}

void VehicleSuspensionCornerSettings::set_anti_roll(float p_anti_roll) {
  anti_roll = p_anti_roll;
}

void VehicleSuspensionCornerSettings::set_wheel_mass(int p_wheel_mass) {
  wheel_mass = p_wheel_mass;
}

void VehicleTireSettings::_bind_methods() {
  ClassDB::bind_method(D_METHOD("set_load_sens0", "load_sens0"),
                       &VehicleTireSettings::set_load_sens0);
  ClassDB::bind_method(D_METHOD("get_load_sens0"),
                       &VehicleTireSettings::get_load_sens0);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "load_sens0"), "set_load_sens0",
               "get_load_sens0");

  ClassDB::bind_method(D_METHOD("set_load_sens1", "load_sens1"),
                       &VehicleTireSettings::set_load_sens1);
  ClassDB::bind_method(D_METHOD("get_load_sens1"),
                       &VehicleTireSettings::get_load_sens1);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "load_sens1"), "set_load_sens1",
               "get_load_sens1");

  ClassDB::bind_method(D_METHOD("set_tire_rated_load", "tire_rated_load"),
                       &VehicleTireSettings::set_tire_rated_load);
  ClassDB::bind_method(D_METHOD("get_tire_rated_load"),
                       &VehicleTireSettings::get_tire_rated_load);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tire_rated_load"),
               "set_tire_rated_load", "get_tire_rated_load");

  ClassDB::bind_method(D_METHOD("set_tire_radius", "tire_radius"),
                       &VehicleTireSettings::set_tire_radius);
  ClassDB::bind_method(D_METHOD("get_tire_radius"),
                       &VehicleTireSettings::get_tire_radius);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tire_radius"), "set_tire_radius",
               "get_tire_radius");
}

void VehicleDrivetrainSettings::_bind_methods() {
  BIND_ENUM_CONSTANT(FWD);
  BIND_ENUM_CONSTANT(RWD);
  BIND_ENUM_CONSTANT(AWD);

  ClassDB::bind_method(
      D_METHOD("set_forward_gear_ratios", "forward_gear_ratios"),
      &VehicleDrivetrainSettings::set_forward_gear_ratios);
  ClassDB::bind_method(D_METHOD("get_forward_gear_ratios"),
                       &VehicleDrivetrainSettings::get_forward_gear_ratios);
  ADD_PROPERTY(
      PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "forward_gear_ratios"),
      "set_forward_gear_ratios", "get_forward_gear_ratios");

  ClassDB::bind_method(
      D_METHOD("set_reverse_gear_ratios", "reverse_gear_ratios"),
      &VehicleDrivetrainSettings::set_reverse_gear_ratios);
  ClassDB::bind_method(D_METHOD("get_reverse_gear_ratios"),
                       &VehicleDrivetrainSettings::get_reverse_gear_ratios);
  ADD_PROPERTY(
      PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "reverse_gear_ratios"),
      "set_reverse_gear_ratios", "get_reverse_gear_ratios");

  ClassDB::bind_method(D_METHOD("set_drive_type", "drive_type"),
                       &VehicleDrivetrainSettings::set_drive_type);
  ClassDB::bind_method(D_METHOD("get_drive_type"),
                       &VehicleDrivetrainSettings::get_drive_type);
  ADD_PROPERTY(PropertyInfo(Variant::INT, "drive_type", PROPERTY_HINT_ENUM,
                            "FWD,RWD,AWD"),
               "set_drive_type", "get_drive_type");

  ClassDB::bind_method(D_METHOD("set_final_drive_ratio", "final_drive_ratio"),
                       &VehicleDrivetrainSettings::set_final_drive_ratio);
  ClassDB::bind_method(D_METHOD("get_final_drive_ratio"),
                       &VehicleDrivetrainSettings::get_final_drive_ratio);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "final_drive_ratio",
                            PROPERTY_HINT_RANGE, "0.0,20.0,0.01"),
               "set_final_drive_ratio", "get_final_drive_ratio");

  ClassDB::bind_method(D_METHOD("set_clutch_max_torque", "clutch_max_torque"),
                       &VehicleDrivetrainSettings::set_clutch_max_torque);
  ClassDB::bind_method(D_METHOD("get_clutch_max_torque"),
                       &VehicleDrivetrainSettings::get_clutch_max_torque);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "clutch_max_torque"),
               "set_clutch_max_torque", "get_clutch_max_torque");

  ClassDB::bind_method(D_METHOD("set_inertia", "inertia"),
                       &VehicleDrivetrainSettings::set_inertia);
  ClassDB::bind_method(D_METHOD("get_inertia"),
                       &VehicleDrivetrainSettings::get_inertia);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "inertia"), "set_inertia",
               "get_inertia");

  ClassDB::bind_method(
      D_METHOD("set_differential_settings", "differential_settings"),
      &VehicleDrivetrainSettings::set_differential_settings);
  ClassDB::bind_method(D_METHOD("get_differential_settings"),
                       &VehicleDrivetrainSettings::get_differential_settings);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "differential_settings",
                            PROPERTY_HINT_RESOURCE_TYPE,
                            "VehicleDifferentialSettings",
                            PROPERTY_USAGE_DEFAULT |
                                PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT),
               "set_differential_settings", "get_differential_settings");
}

PackedFloat32Array VehicleDrivetrainSettings::get_forward_gear_ratios() const {
  return forward_gear_ratios;
}

void VehicleDrivetrainSettings::set_forward_gear_ratios(
    const PackedFloat32Array &p_forward_gear_ratios) {
  forward_gear_ratios = p_forward_gear_ratios;
}

PackedFloat32Array VehicleDrivetrainSettings::get_reverse_gear_ratios() const {
  return reverse_gear_ratios;
}

void VehicleDrivetrainSettings::set_reverse_gear_ratios(
    const PackedFloat32Array &p_reverse_gear_ratios) {
  reverse_gear_ratios = p_reverse_gear_ratios;
}

VehicleDrivetrainSettings::DriveType
VehicleDrivetrainSettings::get_drive_type() const {
  return drive_type;
}

void VehicleDrivetrainSettings::set_drive_type(
    VehicleDrivetrainSettings::DriveType p_drive_type) {
  drive_type = p_drive_type;
}

float VehicleDrivetrainSettings::get_final_drive_ratio() const {
  return final_drive_ratio;
}

void VehicleDrivetrainSettings::set_final_drive_ratio(
    float p_final_drive_ratio) {
  final_drive_ratio = p_final_drive_ratio;
}

float VehicleDrivetrainSettings::get_clutch_max_torque() const {
  return clutch_max_torque;
}

void VehicleDrivetrainSettings::set_clutch_max_torque(
    float p_clutch_max_torque) {
  clutch_max_torque = p_clutch_max_torque;
}

float VehicleDrivetrainSettings::get_inertia() const { return inertia; }

void VehicleDrivetrainSettings::set_inertia(float p_inertia) {
  inertia = p_inertia;
}

Ref<VehicleDifferentialSettings>
VehicleDrivetrainSettings::get_differential_settings() const {
  return differential_settings;
}

void VehicleDrivetrainSettings::set_differential_settings(
    const Ref<VehicleDifferentialSettings> &p_differential_settings) {
  differential_settings = p_differential_settings;
}
