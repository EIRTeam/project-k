#ifndef VEHICLE_CLUTCH_H
#define VEHICLE_CLUTCH_H

#include "core/math/vector2.h"
#include "core/object/ref_counted.h"
class VehicleClutch : public RefCounted {
  GDCLASS(VehicleClutch, RefCounted);
  float friction = 250.0f;
  float clutch_stiffness = 1.0f;
  bool locked = true;

protected:
  static void _bind_methods();

public:
  void process(float p_output_shaft_velocity, float p_engine_angular_velocity,
               float p_gearbox_ratio);
  float get_clutch_torque() const;
  Vector2 get_reaction_torques(float p_av1, float p_av2, float p_t1, float p_t2,
                               float p_slip_torque, float p_kick);

  bool get_locked() const;
  void set_locked(bool p_locked);

  float get_friction() const;
  void set_friction(float p_friction);

  float get_clutch_stiffness() const { return clutch_stiffness; }
  void set_clutch_stiffness(float clutch_stiffness_) { clutch_stiffness = clutch_stiffness_; }
};

#endif // VEHICLE_CLUTCH_H
