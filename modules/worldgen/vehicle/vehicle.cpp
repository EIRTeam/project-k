#include "vehicle.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/string/print_string.h"
#include "worldgen/vehicle/vehicle_drivetrain.h"
#include "worldgen/vehicle/vehicle_settings.h"

void HBVehicle::freewheel(float p_front_brake_torque, float p_rear_brake_torque,
                          float p_delta) {
  clutch_reaction_torque = 0.0f;
  avg_front_spin = 0.0f;
  wheels[WHEEL_FL]->apply_torque(0.0, p_front_brake_torque, 0.0, p_delta);
  wheels[WHEEL_FR]->apply_torque(0.0, p_front_brake_torque, 0.0, p_delta);
  wheels[WHEEL_RL]->apply_torque(0.0, p_rear_brake_torque, 0.0, p_delta);
  wheels[WHEEL_RR]->apply_torque(0.0, p_rear_brake_torque, 0.0, p_delta);
  avg_front_spin +=
      (wheels[WHEEL_FL]->get_spin() + wheels[WHEEL_FR]->get_spin()) * 0.5;
  speedo = avg_front_spin *
           wheels[WHEEL_FL]
               ->get_suspension_settings()
               ->get_tire_settings()
               ->get_tire_radius() *
           3.6;
}

void HBVehicle::engage(float p_engine_torque_out, float p_front_brake_torque,
                       float p_rear_brake_torque, float p_delta) {
  avg_front_spin = 0.0f;
  avg_rear_spin = 0.0f;

  avg_rear_spin +=
      (wheels[WHEEL_RL]->get_spin() + wheels[WHEEL_RR]->get_spin()) * 0.5f;
  avg_front_spin +=
      (wheels[WHEEL_FL]->get_spin() + wheels[WHEEL_FR]->get_spin()) * 0.5f;

  Ref<VehicleDrivetrainSettings> drivetrain_settings =
      settings->get_drivetrain_settings();

  VehicleDrivetrainSettings::DriveType drive_type =
      drivetrain_settings->get_drive_type();

  gearbox_shaft_speed = 0.0f;
  if (drive_type == VehicleDrivetrainSettings::RWD) {
    gearbox_shaft_speed = avg_rear_spin * drivetrain->get_current_gear_ratio();
  } else if (drive_type == VehicleDrivetrainSettings::FWD) {
    gearbox_shaft_speed = avg_front_spin * drivetrain->get_current_gear_ratio();
  } else if (drive_type == VehicleDrivetrainSettings::AWD) {
    gearbox_shaft_speed = (avg_front_spin + avg_rear_spin) * 0.5 *
                          drivetrain->get_current_gear_ratio();
  }

  float speed_error = engine->get_angular_velocity() - gearbox_shaft_speed;
  float clutch_kick = Math::abs(speed_error) * 0.2f;
  float tr = drivetrain->get_reaction_torque();
  float clutch_slip_torque = 0.8f * clutch->get_friction();
  Vector2 reaction_torques = clutch->get_reaction_torques(
      engine->get_angular_velocity(), gearbox_shaft_speed, p_engine_torque_out,
      tr, clutch_slip_torque, clutch_kick);

  if (clutch->get_locked()) {
    reaction_torques.x = p_engine_torque_out;
  }

  drive_reaction_torque = reaction_torques.x * (1.0f - clutch_input);
  clutch_reaction_torque = reaction_torques.y * (1.0f - clutch_input);

  float net_drive = drive_reaction_torque;

  VehicleDrivetrain::DrivetrainProcessResults drivetrain_results;

  drivetrain->process(
      {
          .drive_torque = net_drive,
          .rear_brake_torque = p_rear_brake_torque,
          .front_brake_torque = p_front_brake_torque,
          .engine_inertia = engine->get_settings()->get_inertia(),
          .clutch_input = clutch_input,
          .wheels = wheels,
          .delta = p_delta,
          .shift_request = shift_request,
      },
      drivetrain_results);
  avg_front_spin = drivetrain_results.avg_front_spin;
  avg_rear_spin = drivetrain_results.avg_rear_spin;

  speedo = avg_front_spin *
           wheels[WHEEL_FL]
               ->get_suspension_settings()
               ->get_tire_settings()
               ->get_tire_radius() *
           3.6;
  shift_request = VehicleDrivetrain::ShiftRequest::NONE;
}

void HBVehicle::drag_force() {
  // TODO: Make customizable
  const float CD = 0.3f;
  const float FRONTAL_AREA = 2.0f;
  const float AIR_DENSITY = 1.225f;
  float spd = sqrt(x_vel * x_vel + z_vel * z_vel);
  float cdrag = 0.5 * CD * FRONTAL_AREA * AIR_DENSITY;

  // fdrag.y is positive in this case because forward is -z in godot
  Vector2 fdrag;
  fdrag.y = CLAMP(cdrag * z_vel * spd, -100000, 100000);
  fdrag.x = CLAMP(-cdrag * x_vel * spd, -100000, 100000);

  apply_central_force(get_global_transform().basis.get_column(2).normalized() *
                      fdrag.y);
  apply_central_force(get_global_transform().basis.get_column(0).normalized() *
                      fdrag.x);
}

void HBVehicle::_bind_methods() {
  ClassDB::bind_method(D_METHOD("set_throttle_input", "throttle_input"),
                       &HBVehicle::set_throttle_input);
  ClassDB::bind_method(D_METHOD("set_brake_input", "brake_input"),
                       &HBVehicle::set_brake_input);
  ClassDB::bind_method(D_METHOD("set_clutch_input", "clutch_input"),
                       &HBVehicle::set_clutch_input);
  ClassDB::bind_method(D_METHOD("set_steering_input", "steering_input"),
                       &HBVehicle::set_steering_input);
  ClassDB::bind_method(
      D_METHOD("setup", "wheel_fl", "wheel_fr", "wheel_rl", "wheel_rr"),
      &HBVehicle::setup);

  ClassDB::bind_method(D_METHOD("set_settings", "settings"),
                       &HBVehicle::set_settings);
  ClassDB::bind_method(D_METHOD("get_settings"), &HBVehicle::get_settings);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "settings",
                            PROPERTY_HINT_RESOURCE_TYPE, "VehicleSettings",
                            PROPERTY_USAGE_DEFAULT |
                                PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT),
               "set_settings", "get_settings");
  ClassDB::bind_method(D_METHOD("process", "delta"), &HBVehicle::process);
  ClassDB::bind_method(D_METHOD("set_shift_request", "shift_request"),
                       &HBVehicle::set_shift_request);
  ClassDB::bind_method(D_METHOD("get_rpm"), &HBVehicle::get_rpm);
}

void HBVehicle::process(float p_delta) {
  float front_brake_torque;
  float rear_brake_torque;
  get_brake_torques(get_brake_input(), front_brake_torque, rear_brake_torque);

  rear_brake_torque += handbrake_input * settings->get_max_handbrake_torque();
  local_velocity = get_global_basis().xform_inv(
      (get_global_transform().origin - prev_pos) / p_delta);
  prev_pos = get_global_position();

  z_vel = -local_velocity.z;
  x_vel = local_velocity.x;

  steering_amount = Math::move_toward(steering_amount, steering_input,
                                      settings->get_steering_speed() * p_delta);

  wheels[WHEEL_FL]->steer(steering_amount, settings->get_max_steer_angle(),
                          settings->get_ackermann());
  wheels[WHEEL_FR]->steer(steering_amount, settings->get_max_steer_angle(),
                          -settings->get_ackermann());

  engine_torque_out = engine->get_settings()->get_torque_at_rpm(
      engine->get_rpm(), throttle_input);
  engine_net_torque =
      engine->advance(p_delta, throttle_input, clutch_reaction_torque);

  // TODO: Automatic shifting

  if (shift_request == VehicleDrivetrain::SHIFT_UP) {
    drivetrain->shift_up();
  } else if (shift_request == VehicleDrivetrain::SHIFT_DOWN) {
    drivetrain->shift_down();
  }

  if (drivetrain->get_current_gear() == 0) {
    freewheel(front_brake_torque, rear_brake_torque, p_delta);
  } else {
    engage(engine_torque_out, front_brake_torque, rear_brake_torque, p_delta);
  }

  std::array<float, WHEEL_MAX> prev_suspension_compression;
  for (int i = 0; i < WHEEL_MAX; i++) {
    prev_suspension_compression[i] = suspension_compression[i];
  }
  suspension_compression[WHEEL_RL] = wheels[WHEEL_RL]->apply_forces(
      this, prev_suspension_compression[WHEEL_RR], p_delta);
  suspension_compression[WHEEL_RR] = wheels[WHEEL_RR]->apply_forces(
      this, prev_suspension_compression[WHEEL_RL], p_delta);
  suspension_compression[WHEEL_FL] = wheels[WHEEL_FL]->apply_forces(
      this, prev_suspension_compression[WHEEL_FR], p_delta);
  suspension_compression[WHEEL_FR] = wheels[WHEEL_FR]->apply_forces(
      this, prev_suspension_compression[WHEEL_FL], p_delta);

  // drag_force();
}

void HBVehicle::set_shift_request(
    VehicleDrivetrain::ShiftRequest p_set_shift_request) {
  shift_request = p_set_shift_request;
}

void HBVehicle::get_brake_torques(float p_brake_input, float &r_front,
                                  float &r_rear) const {
  const float max_brake_torque = settings->get_max_brake_torque();
  r_front =
      p_brake_input * max_brake_torque * settings->get_brake_front_share();
  r_rear = p_brake_input * max_brake_torque *
           (1 - settings->get_brake_front_share());
}

void HBVehicle::setup(VehicleSuspensionCorner *p_wheel_fl,
                      VehicleSuspensionCorner *p_wheel_fr,
                      VehicleSuspensionCorner *p_wheel_rl,
                      VehicleSuspensionCorner *p_wheel_rr) {
  initialized = true;

  wheels[WHEEL_FL] = p_wheel_fl;
  wheels[WHEEL_FR] = p_wheel_fr;
  wheels[WHEEL_RL] = p_wheel_rl;
  wheels[WHEEL_RR] = p_wheel_rr;

  wheels[WHEEL_FL]->set_suspension_settings(settings->get_wheel_fl());
  wheels[WHEEL_FR]->set_suspension_settings(settings->get_wheel_fr());
  wheels[WHEEL_RL]->set_suspension_settings(settings->get_wheel_rl());
  wheels[WHEEL_RR]->set_suspension_settings(settings->get_wheel_rr());

  set_mass(settings->get_mass());

  engine.instantiate();
  engine->set_settings(settings->get_engine_settings());

  drivetrain.instantiate();
  drivetrain->set_settings(settings->get_drivetrain_settings());

  clutch.instantiate();
  clutch->set_friction(
      settings->get_drivetrain_settings()->get_clutch_max_torque());
}

int HBVehicle::get_rpm() const { return engine->get_rpm(); }
