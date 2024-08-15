#ifndef VEHICLE_SUSPENSION_CORNER_H
#define VEHICLE_SUSPENSION_CORNER_H

#include "scene/3d/node_3d.h"
#include "worldgen/vehicle/vehicle_settings.h"

class RigidBody3D;

class VehicleSuspensionCorner : public Node3D {
  GDCLASS(VehicleSuspensionCorner, Node3D);
  Ref<VehicleSuspensionCornerSettings> suspension_settings;
  Vector3 prev_pos;

  float spin = 0.0f;
  float current_spring_length = 0.0f;
  float prev_spring_load_mm = 0.0f;
  float rolling_resistance = 0.0f;
  bool process_collision(Vector3 &r_position, Vector3 &r_normal) const;

protected:
  static void _bind_methods();

public:
  Vector3 force_vec;
  float apply_forces(RigidBody3D *p_car, float p_opposite_comp, float p_delta);
  float apply_torque(float p_drive_torque, float p_brake_torque,
                     float p_drive_inertia, float p_delta);
  Ref<VehicleSuspensionCornerSettings> get_suspension_settings() const {
    return suspension_settings;
  }
  void set_suspension_settings(
      const Ref<VehicleSuspensionCornerSettings> &suspension_settings_) {
    suspension_settings = suspension_settings_;
  }
  void steer(float p_input, float p_max_steer, float p_ackermann);

  float get_spin() const { return spin; }
  void set_spin(float spin_) { spin = spin_; }
  float get_reaction_torque() const;

  float get_rolling_resistance() const;
  void set_rolling_resistance(float p_rolling_resistance);
  float get_wheel_inertia() const;
  float get_current_spring_length() const { return current_spring_length; };
};

#endif // VEHICLE_SUSPENSION_CORNER_H
