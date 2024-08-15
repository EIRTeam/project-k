#ifndef VEHICLE_ENGINE_H
#define VEHICLE_ENGINE_H

#include "core/object/ref_counted.h"
#include "worldgen/vehicle/vehicle_settings.h"

class VehicleEngine : public RefCounted {
  GDCLASS(VehicleEngine, RefCounted);
  Ref<VehicleEngineSettings> settings;

  float time = 0.0f;
  float last_limiter_trigger = -1000.0f;

  float rpm = 0.0f;

protected:
  static void _bind_methods();

public:
  float advance(float p_delta, float p_throttle,
                float p_clutch_reaction_torque);
  float get_angular_velocity() const;
  float get_rpm() const;

  Ref<VehicleEngineSettings> get_settings() const;
  void set_settings(Ref<VehicleEngineSettings> p_settings);
};

#endif // VEHICLE_ENGINE_H
