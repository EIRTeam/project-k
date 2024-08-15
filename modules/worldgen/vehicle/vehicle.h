#ifndef VEHICLE_H
#define VEHICLE_H

#include "scene/3d/physics/rigid_body_3d.h"
#include "worldgen/vehicle/vehicle_clutch.h"
#include "worldgen/vehicle/vehicle_drivetrain.h"
#include "worldgen/vehicle/vehicle_engine.h"
#include "worldgen/vehicle/vehicle_settings.h"
#include "worldgen/vehicle/vehicle_suspension_corner.h"
#include <array>

class HBVehicle : public RigidBody3D {
  GDCLASS(HBVehicle, RigidBody3D);

public:
  enum WheelPositions { WHEEL_FL, WHEEL_FR, WHEEL_RL, WHEEL_RR, WHEEL_MAX };

private:
  Ref<VehicleSettings> settings;
  std::array<VehicleSuspensionCorner *, 4> wheels;
  std::array<float, WHEEL_MAX> suspension_compression = {0.5f, 0.5f, 0.5f,
                                                         0.5f};

  Ref<VehicleEngine> engine;
  Ref<VehicleDrivetrain> drivetrain;
  Ref<VehicleClutch> clutch;

  bool initialized = false;

  float throttle_input = 0.0f;
  float brake_input = 0.0f;
  float handbrake_input = 0.0f;
  float steering_input = 0.0f;
  float clutch_input = 0.0f;

  Vector3 local_velocity;
  Vector3 prev_pos;
  float z_vel = 0.0f;
  float x_vel = 0.0f;
  float steering_amount = 0.0f;
  float engine_torque_out = 0.0f;
  float engine_net_torque = 0.0f;
  float clutch_reaction_torque = 0.0f;
  float avg_front_spin = 0.0f;
  float avg_rear_spin = 0.0f;
  float speedo = 0.0f;
  float drive_reaction_torque = 0.0f;
  float gearbox_shaft_speed = 0.0f;
  VehicleDrivetrain::ShiftRequest shift_request =
      VehicleDrivetrain::ShiftRequest::NONE;

private:
  void freewheel(float p_front_brake_torque, float p_rear_brake_torque,
                 float p_delta);
  void engage(float p_engine_torque_out, float p_front_brake_torque,
              float p_rear_brake_torque, float p_delta);
  void drag_force();

protected:
  static void _bind_methods();

public:
  void process(float p_delta);
  void set_shift_request(VehicleDrivetrain::ShiftRequest p_set_shift_request);
  void get_brake_torques(float p_brake_input, float &r_front,
                         float &r_rear) const;
  void setup(VehicleSuspensionCorner *p_wheel_fl,
             VehicleSuspensionCorner *p_wheel_fr,
             VehicleSuspensionCorner *p_wheel_rl,
             VehicleSuspensionCorner *p_wheel_rr);

  float get_throttle_input() const { return throttle_input; }
  void set_throttle_input(float throttle_input_) {
    throttle_input = throttle_input_;
  }

  float get_brake_input() const { return brake_input; }
  void set_brake_input(float brake_input_) { brake_input = brake_input_; }

  float get_handbrake_input() const { return handbrake_input; }
  void set_handbrake_input(float handbrake_input_) {
    handbrake_input = handbrake_input_;
  }

  float get_steering_input() const { return steering_input; }
  void set_steering_input(float steering_input_) {
    steering_input = steering_input_;
  }

  float get_clutch_input() const { return clutch_input; }
  void set_clutch_input(float clutch_input_) { clutch_input = clutch_input_; }

  Ref<VehicleSettings> get_settings() const { return settings; }
  void set_settings(const Ref<VehicleSettings> &p_settings) {
    settings = p_settings;
  }

  int get_rpm() const;
  float get_drive_reaction_torque() const { return drive_reaction_torque; }

  friend class VehicleDebugger;
};

#endif // VEHICLE_H
