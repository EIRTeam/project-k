#ifndef VEHICLE_SETTINGS_H
#define VEHICLE_SETTINGS_H

#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/variant/variant.h"
#include "scene/resources/curve.h"

class VehicleDifferentialSettings : public Resource {
  GDCLASS(VehicleDifferentialSettings, Resource);
  float preload = 50.0f;
  float power_ratio = 2.0f;
  float coast_ratio = 1.0f;

public:
  enum DifferentialType { LIMITED_SLIP, OPEN, CLOSED };

private:
  DifferentialType type;

protected:
  static void _bind_methods();

public:
  DifferentialType get_type() const { return type; }
  void set_type(DifferentialType p_type) { type = p_type; }

  float get_coast_ratio() const { return coast_ratio; }
  void set_coast_ratio(float p_coast_ratio) { coast_ratio = p_coast_ratio; }

  float get_power_ratio() const { return power_ratio; }
  void set_power_ratio(float p_power_ratio) { power_ratio = p_power_ratio; }

  float get_preload() const { return preload; }
  void set_preload(float p_preload) { preload = p_preload; }
};

class VehicleDrivetrainSettings : public Resource {
  GDCLASS(VehicleDrivetrainSettings, Resource);
  PackedFloat32Array forward_gear_ratios;
  PackedFloat32Array reverse_gear_ratios;
  float final_drive_ratio = 0.0f;
  float inertia = 0.018f;
  float clutch_max_torque = 320.0f;

  Ref<VehicleDifferentialSettings> differential_settings;

public:
  enum DriveType { FWD, RWD, AWD };

private:
  DriveType drive_type = RWD;

protected:
  static void _bind_methods();

public:
  PackedFloat32Array get_forward_gear_ratios() const;
  void set_forward_gear_ratios(const PackedFloat32Array &p_forward_gear_ratios);

  PackedFloat32Array get_reverse_gear_ratios() const;
  void set_reverse_gear_ratios(const PackedFloat32Array &p_reverse_gear_ratios);

  DriveType get_drive_type() const;
  void set_drive_type(DriveType p_drive_type);

  float get_final_drive_ratio() const;
  void set_final_drive_ratio(float p_final_drive_ratio);

  float get_clutch_max_torque() const;
  void set_clutch_max_torque(float p_clutch_max_torque);

  float get_inertia() const;
  void set_inertia(float p_inertia);

  Ref<VehicleDifferentialSettings> get_differential_settings() const;
  void set_differential_settings(
      const Ref<VehicleDifferentialSettings> &p_differential_settings);
};

class VehicleTireSettings : public Resource {
  GDCLASS(VehicleTireSettings, Resource);
  float tire_radius = 0.3f;
  float tire_width = 0.205f;
  float tire_rated_load = 5500.0f;
  float load_sens0 = 1.7f;
  float load_sens1 = 0.9f;
  float max_tire_temp = 150.0f;
  float opt_tire_temp = 90.0f;
  float heating_multiplier = 0.0015f;
  float cooling_multiplier = -0.25f;

protected:
  static void _bind_methods();

public:
  float calculate_load_sensitivity(float p_normal_load) const {
    float load_factor = p_normal_load / get_tire_rated_load();
    return CLAMP(Math::lerp(get_load_sens0(), get_load_sens1(), load_factor),
                 0.2, get_load_sens0());
  }

  float get_load_sens0() const { return load_sens0; }
  void set_load_sens0(float load_sens0_) { load_sens0 = load_sens0_; }

  float get_load_sens1() const { return load_sens1; }
  void set_load_sens1(float load_sens1_) { load_sens1 = load_sens1_; }

  float get_tire_rated_load() const { return tire_rated_load; }
  void set_tire_rated_load(float tire_rated_load_) {
    tire_rated_load = tire_rated_load_;
  }
  virtual Vector3 update_forces(const Vector2 p_slip_ratio,
                                const float p_normal_load,
                                const float p_surface_mu) const = 0;

  float get_tire_radius() const { return tire_radius; }
  void set_tire_radius(float tire_radius_) { tire_radius = tire_radius_; }
};

class VehicleSuspensionCornerSettings : public Resource {
  GDCLASS(VehicleSuspensionCornerSettings, Resource);
  float spring_travel = 0.2f;
  float spring_rate = 45.0f;
  float bump = 3.5f;
  float rebound = 4.0f;
  float anti_roll = 0.0f;
  int wheel_mass = 15;
  Ref<VehicleTireSettings> tire_settings;

protected:
  static void _bind_methods();

public:
  float get_spring_travel() const;
  void set_spring_travel(float p_spring_travel);

  Ref<VehicleTireSettings> get_tire_settings() const;
  void set_tire_settings(const Ref<VehicleTireSettings> &p_tire_settings);

  float get_spring_rate() const;
  void set_spring_rate(float p_spring_rate);

  float get_bump() const;
  void set_bump(float p_bump);

  float get_rebound() const;
  void set_rebound(float p_rebound);

  float get_anti_roll() const;
  void set_anti_roll(float p_anti_roll);

  int get_wheel_mass() const { return wheel_mass; }
  void set_wheel_mass(int p_wheel_mass);
};

class VehicleEngineSettings : public Resource {
  GDCLASS(VehicleEngineSettings, Resource);

  int rpm_limit = 8000;
  int limiter_hz = 12;
  PackedVector2Array torque_curve;
  PackedVector2Array throttle_map;

  int coast_ref_rpm = 7250;
  int coast_ref_torque = 65;
  float coast_curve_non_linearity = 0;

  float inertia = 0.13f;
  int idle_rpm = 850;

  Ref<Curve> throttle_map_baked;
  Ref<Curve> torque_curve_baked;
  float torque_curve_max_rpm = 0.0f;
  Ref<Curve> coast_torque_curve_baked;

  bool torque_curve_dirty = true;
  void _update_torque_curve();
  bool coast_curve_dirty = true;

protected:
  static void _bind_methods();

public:
  int get_rpm_limit() const;
  void set_rpm_limit(int p_rpm_limit);

  PackedVector2Array get_torque_curve() const;
  void set_torque_curve(PackedVector2Array p_torque_curve);

  PackedVector2Array get_throttle_map() const;
  void set_throttle_map(PackedVector2Array p_throttle_map);

  int get_coast_ref_rpm() const;
  void set_coast_ref_rpm(int p_coast_ref_rpm);

  int get_coast_ref_torque() const;
  void set_coast_ref_torque(int p_coast_ref_torque);

  float get_coast_curve_non_linearity() const;
  void set_coast_curve_non_linearity(float p_coast_curve_non_linearity);

  float get_inertia() const;
  void set_inertia(float p_inertia);

  int get_idle_rpm() const;
  void set_idle_rpm(int p_idle_rpm);

  float sample_throttle_map(float p_throttle) const;
  float get_on_throttle_torque_at_rpm(float p_rpm) const;
  float get_coast_torque_at_rpm(float p_rpm) const;
  float get_torque_at_rpm(float p_rpm, float p_throttle) const;

  int get_limiter_hz() const;
  void set_limiter_hz(int p_limiter_hz);
};

class VehicleCurveModelTireSettings : public VehicleTireSettings {
  GDCLASS(VehicleCurveModelTireSettings, VehicleTireSettings);
  float tire_stiffness = 0.2f;
  static constexpr float MIN_PEAK_SA_START_DEG = 3.0f;
  static constexpr float MAX_PEAK_SA_START_DEG = 12.0f;

  static constexpr float MIN_DELTA_SA_DEG = 1.0f;
  static constexpr float MAX_DELTA_SA_DEG = 4.0f;

  Ref<Curve> buildup;
  Ref<Curve> falloff;

protected:
  static void _bind_methods();

public:
  float get_tire_stiffness() const { return tire_stiffness; }
  void set_tire_stiffness(float p_tire_stiffness) {
    tire_stiffness = p_tire_stiffness;
  }
  virtual Vector3 update_forces(const Vector2 p_slip_ratio,
                                const float p_normal_load,
                                const float p_surface_mu) const override;

  Ref<Curve> get_falloff() const;
  void set_falloff(const Ref<Curve> &p_falloff);

  Ref<Curve> get_buildup() const;
  void set_buildup(const Ref<Curve> &p_buildup);
};

class VehicleSettings : public Resource {
  GDCLASS(VehicleSettings, Resource);
  int mass = 1000;
  float brake_front_share = 0.6f;
  float max_brake_torque = 1400.0f;
  float max_handbrake_torque;
  float max_steer_angle = 0.4f;
  float ackermann = 0.15f;
  float steering_speed = 1.0f;
  Ref<VehicleSuspensionCornerSettings> wheel_fl;
  Ref<VehicleSuspensionCornerSettings> wheel_fr;
  Ref<VehicleSuspensionCornerSettings> wheel_rl;
  Ref<VehicleSuspensionCornerSettings> wheel_rr;
  Ref<VehicleEngineSettings> engine_settings;
  Ref<VehicleDrivetrainSettings> drivetrain_settings;

protected:
  static void _bind_methods();

public:
  Ref<VehicleSuspensionCornerSettings> get_wheel_fl() const;
  void set_wheel_fl(const Ref<VehicleSuspensionCornerSettings> &p_wheel_fl);

  Ref<VehicleSuspensionCornerSettings> get_wheel_fr() const;
  void set_wheel_fr(const Ref<VehicleSuspensionCornerSettings> &p_wheel_fr);

  Ref<VehicleSuspensionCornerSettings> get_wheel_rl() const;
  void set_wheel_rl(const Ref<VehicleSuspensionCornerSettings> &p_wheel_rl);

  Ref<VehicleSuspensionCornerSettings> get_wheel_rr() const;
  void set_wheel_rr(const Ref<VehicleSuspensionCornerSettings> &p_wheel_rr);

  int get_mass() const;
  void set_mass(int p_mass);

  Ref<VehicleEngineSettings> get_engine_settings() const;
  void set_engine_settings(const Ref<VehicleEngineSettings> &p_engine_settings);

  float get_brake_front_share() const;
  void set_brake_front_share(float p_brake_front_share);

  float get_max_brake_torque() const;
  void set_max_brake_torque(float p_max_brake_torque);

  float get_max_handbrake_torque() const;
  void set_max_handbrake_torque(float p_max_handbrake_torque);

  float get_steering_speed() const;
  void set_steering_speed(float p_steering_speed);

  float get_ackermann() const;
  void set_ackermann(float p_ackermann);

  float get_max_steer_angle() const;
  void set_max_steer_angle(float p_max_steer_angle);

  Ref<VehicleDrivetrainSettings> get_drivetrain_settings() const;
  void set_drivetrain_settings(
      const Ref<VehicleDrivetrainSettings> &p_drivetrain_settings);
};

VARIANT_ENUM_CAST(VehicleDifferentialSettings::DifferentialType);
VARIANT_ENUM_CAST(VehicleDrivetrainSettings::DriveType);

#endif // VEHICLE_SETTINGS_H
