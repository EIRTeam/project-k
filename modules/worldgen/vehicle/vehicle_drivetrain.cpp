#include "vehicle_drivetrain.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"
#include "worldgen/vehicle/vehicle.h"
#include "worldgen/vehicle/vehicle_settings.h"
#include "worldgen/vehicle/vehicle_suspension_corner.h"

void VehicleDrivetrain::differential(
    float p_torque, float p_brake_torque,
    VehicleSuspensionCorner *p_drive_wheels[2],
    Ref<VehicleDifferentialSettings> p_diff_settings, float p_drive_inertia,
    float p_delta) {
  DifferentialState diff_state = DifferentialState::LOCKED;
  float tr1 = Math::abs(p_drive_wheels[0]->get_reaction_torque());
  float tr2 = Math::abs(p_drive_wheels[1]->get_reaction_torque());

  float delta_torque = 0.0f;
  float bias = 0.0f;
  if (tr1 >= tr2) {
    bias = tr1 / tr2;
  } else {
    bias = tr2 / tr1;
  }

  delta_torque = tr1 - tr2;
  float t1 = p_torque * 0.5f;
  float t2 = p_torque * 0.5f;

  const float gear_ratio = get_current_gear_ratio();

  float ratio = p_diff_settings->get_power_ratio();

  if (p_torque * SIGN(gear_ratio) < 0.0f) {
    ratio = p_diff_settings->get_coast_ratio();
  }

  if (p_diff_settings->get_type() == VehicleDifferentialSettings::OPEN) {
    diff_state = DifferentialState::OPEN;
  } else if (p_diff_settings->get_type() ==
             VehicleDifferentialSettings::CLOSED) {
    diff_state = DifferentialState::LOCKED;
  } else {
    // LSD
    if (Math::abs(delta_torque) > p_diff_settings->get_preload() &&
        bias >= ratio) {
      diff_state = DifferentialState::SLIPPING;
    }
  }
  switch (diff_state) {
  case OPEN: {
    float diff_sum = 0.0f;
    t2 *= diff_split;
    t1 *= (1.0f - diff_split);

    diff_sum += p_drive_wheels[0]->apply_torque(t1, p_brake_torque * 0.5,
                                                p_drive_inertia, p_delta);
    diff_sum -= p_drive_wheels[1]->apply_torque(t2, p_brake_torque * 0.5,
                                                p_drive_inertia, p_delta);
    diff_split = 0.5 * (CLAMP(diff_sum, -1.0f, 1.0f) + 1.0f);
  } break;
  case SLIPPING: {
    diff_clutch->set_friction(p_diff_settings->get_preload());
    Vector2 diff_torques = diff_clutch->get_reaction_torques(
        p_drive_wheels[0]->get_spin(), p_drive_wheels[1]->get_spin(), tr1, tr2,
        p_diff_settings->get_preload() * ratio, 0.0f);
    t1 += diff_torques.x;
    t2 += diff_torques.y;

    p_drive_wheels[0]->apply_torque(t1, p_brake_torque * 0.5f, p_drive_inertia,
                                    p_delta);
    p_drive_wheels[1]->apply_torque(t2, p_brake_torque * 0.5f, p_drive_inertia,
                                    p_delta);
  } break;
  case LOCKED: {
    float net_torque = p_drive_wheels[0]->get_reaction_torque() +
                       p_drive_wheels[1]->get_reaction_torque();
    print_line(net_torque);
    net_torque += t1 + t2;

    float spin = 0.0f;
    float avg_spin =
        (p_drive_wheels[0]->get_spin() + p_drive_wheels[1]->get_spin()) * 0.5f;
    float rolling_resistance = p_drive_wheels[0]->get_rolling_resistance() +
                               p_drive_wheels[1]->get_rolling_resistance();

    if (Math::abs(avg_spin) < 5.0f && p_brake_torque > Math::abs(net_torque)) {
      spin = 0.0;
    } else {
      net_torque -= (p_brake_torque + rolling_resistance) * SIGN(avg_spin);
    }

    spin = avg_spin +
           p_delta * net_torque /
               (p_drive_wheels[0]->get_wheel_inertia() + p_drive_inertia +
                p_drive_wheels[1]->get_wheel_inertia());
    p_drive_wheels[0]->set_spin(spin);
    p_drive_wheels[1]->set_spin(spin);
  } break;
  }
}

void VehicleDrivetrain::shift_up() {
  if (current_gear < settings->get_forward_gear_ratios().size()) {
    current_gear += 1;
  }
}

void VehicleDrivetrain::shift_down() {
  if (current_gear > -settings->get_reverse_gear_ratios().size()) {
    current_gear -= 1;
  }
}

void VehicleDrivetrain::_bind_methods() {
  BIND_ENUM_CONSTANT(NONE);
  BIND_ENUM_CONSTANT(SHIFT_UP);
  BIND_ENUM_CONSTANT(SHIFT_DOWN);
}

float VehicleDrivetrain::get_ratio_for_gear(int p_gear) const {
  if (p_gear > 0 && p_gear <= settings->get_forward_gear_ratios().size()) {
    return settings->get_forward_gear_ratios()[p_gear - 1];
  }
  if (p_gear < 0 &&
      Math::abs(p_gear) <= settings->get_reverse_gear_ratios().size()) {
    return settings->get_reverse_gear_ratios()[Math::abs(p_gear) - 1];
  }

  // Neutral or invalid gear
  return 0.0f;
}

float VehicleDrivetrain::get_current_gear_ratio() const {
  return get_ratio_for_gear(current_gear) * settings->get_final_drive_ratio();
}

float VehicleDrivetrain::get_reaction_torque() const { return reaction_torque; }

void VehicleDrivetrain::process(const DrivetrainProcessParams &p_process_params,
                                DrivetrainProcessResults &r_results) {
  const float gear_ratio = get_current_gear_ratio();
  VehicleSuspensionCorner *rear_wheels[2] = {
      p_process_params.wheels[HBVehicle::WHEEL_RL],
      p_process_params.wheels[HBVehicle::WHEEL_RR]};
  VehicleSuspensionCorner *front_wheels[2] = {
      p_process_params.wheels[HBVehicle::WHEEL_FL],
      p_process_params.wheels[HBVehicle::WHEEL_FR]};

  r_results.avg_rear_spin =
      (p_process_params.wheels[HBVehicle::WHEEL_RL]->get_spin() +
       p_process_params.wheels[HBVehicle::WHEEL_RR]->get_spin()) *
      0.5f;
  r_results.avg_front_spin =
      (p_process_params.wheels[HBVehicle::WHEEL_FL]->get_spin() +
       p_process_params.wheels[HBVehicle::WHEEL_FR]->get_spin()) *
      0.5f;

  const float gear_ratio_sq = Math::abs(gear_ratio) * Math::abs(gear_ratio);

  float drive_inertia = (p_process_params.engine_inertia +
                         gear_ratio_sq * settings->get_inertia());
  float drive_torque = p_process_params.drive_torque * gear_ratio;

  if (settings->get_drive_type() == VehicleDrivetrainSettings::RWD) {
    differential(drive_torque, p_process_params.rear_brake_torque, rear_wheels,
                 settings->get_differential_settings(), drive_inertia,
                 p_process_params.delta);
    front_wheels[0]->apply_torque(0.0f,
                                  p_process_params.front_brake_torque * 0.5f,
                                  0.0f, p_process_params.delta);
    front_wheels[1]->apply_torque(0.0f,
                                  p_process_params.front_brake_torque * 0.5f,
                                  0.0f, p_process_params.delta);
    reaction_torque = (rear_wheels[0]->get_reaction_torque() +
                       rear_wheels[1]->get_reaction_torque()) *
                      0.5f;
    reaction_torque *= (1.0f / gear_ratio);
  } else if (settings->get_drive_type() == VehicleDrivetrainSettings::FWD) {
    differential(drive_torque, p_process_params.front_brake_torque,
                 front_wheels, settings->get_differential_settings(),
                 drive_inertia, p_process_params.delta);
    rear_wheels[0]->apply_torque(0.0f,
                                 p_process_params.rear_brake_torque * 0.5f,
                                 0.0f, p_process_params.delta);
    rear_wheels[1]->apply_torque(0.0f,
                                 p_process_params.rear_brake_torque * 0.5f,
                                 0.0f, p_process_params.delta);
    reaction_torque = (front_wheels[0]->get_reaction_torque() +
                       front_wheels[1]->get_reaction_torque()) *
                      0.5f;
    reaction_torque *= (1.0f / gear_ratio);
  }

  // TODO: AWD
}

VehicleDrivetrain::VehicleDrivetrain() { diff_clutch.instantiate(); }
