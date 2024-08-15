#include "vehicle_clutch.h"
#include "core/object/class_db.h"
#include "core/object/object.h"

void VehicleClutch::_bind_methods() {
  ClassDB::bind_method(D_METHOD("get_reaction_torques", "av1", "av2", "t1",
                                "t2", "slip_torque", "kick"),
                       &VehicleClutch::get_reaction_torques);
  ClassDB::bind_method(D_METHOD("set_locked", "locked"),
                       &VehicleClutch::set_locked);
  ClassDB::bind_method(D_METHOD("get_locked"), &VehicleClutch::get_locked);
  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "locked"), "set_locked",
               "get_locked");

  ClassDB::bind_method(D_METHOD("set_friction", "friction"),
                       &VehicleClutch::set_friction);
  ClassDB::bind_method(D_METHOD("get_friction"), &VehicleClutch::get_friction);
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "friction"), "set_friction",
               "get_friction");
  ClassDB::bind_method(D_METHOD("get_clutch_torque"),
                       &VehicleClutch::get_clutch_torque);
  ClassDB::bind_method(D_METHOD("process"), &VehicleClutch::process);
}

Vector2 VehicleClutch::get_reaction_torques(float p_av1, float p_av2,
                                            float p_t1, float p_t2,
                                            float p_slip_torque, float p_kick) {
  float clutch_torque = friction + p_kick;
  float delta_torque = p_t1 - p_t2;
  float delta_av = p_av1 - p_av2;
  Vector2 reaction_torques = Vector2();

  // Locked situations are handled in car and drivetrain scripts atm
  if (locked) {
    if (Math::absf(delta_torque) >= p_slip_torque) {
      locked = false;
    }
  } else {
    if (Math::absf(delta_av) < 0.5) {
      locked = true;
    }
  }
  if (p_av1 < p_av2) {
    reaction_torques.x = -clutch_torque;
    reaction_torques.y = clutch_torque;
  } else {
    reaction_torques.x = clutch_torque;
    reaction_torques.y = -clutch_torque;
  }
  return reaction_torques;
}

bool VehicleClutch::get_locked() const { return locked; }

void VehicleClutch::set_locked(bool p_locked) { locked = p_locked; }

float VehicleClutch::get_friction() const { return friction; }

void VehicleClutch::set_friction(float p_friction) { friction = p_friction; }
