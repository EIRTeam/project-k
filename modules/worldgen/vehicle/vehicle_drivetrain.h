#ifndef VEHICLE_DRIVETRAIN_H
#define VEHICLE_DRIVETRAIN_H

#include "core/object/ref_counted.h"
#include "core/variant/binder_common.h"
#include "vehicle_settings.h"
#include "vehicle_suspension_corner.h"
#include "worldgen/vehicle/vehicle_clutch.h"
#include <array>

class VehicleDrivetrain : public RefCounted {
  GDCLASS(VehicleDrivetrain, RefCounted);
  Ref<VehicleDrivetrainSettings> settings;


  int current_gear = 0;
  float reaction_torque = 0.0f;
  float diff_split = 0.5f;
  Ref<VehicleClutch> diff_clutch;

public:
  enum ShiftRequest { NONE, SHIFT_UP, SHIFT_DOWN };

private:
  void differential(float p_torque, float p_brake_torque,
                    VehicleSuspensionCorner *p_drive_wheels[2],
                    Ref<VehicleDifferentialSettings> p_diff_settings,
                    float p_drive_inertia, float p_delta);

protected:
  static void _bind_methods();

public:
  enum DifferentialState { LOCKED, SLIPPING, OPEN };
  Ref<VehicleDrivetrainSettings> get_settings() const { return settings; }
  void set_settings(const Ref<VehicleDrivetrainSettings> &settings_) {
    settings = settings_;
  }

  void shift_up();
  void shift_down();
  float get_ratio_for_gear(int p_gear) const;
  float get_current_gear_ratio() const;
  int get_current_gear() const { return current_gear; }
  void set_current_gear(int current_gear_) { current_gear = current_gear_; }
  float get_reaction_torque() const;
  void set_reaction_torque(float p_reaction_torque) {
    reaction_torque = p_reaction_torque;
  }
  struct DrivetrainProcessParams {
    float drive_torque;
    float rear_brake_torque;
    float front_brake_torque;
    float engine_inertia;
    float clutch_input;
    std::array<VehicleSuspensionCorner *, 4> wheels;
    float delta;
    ShiftRequest shift_request;
  };
  struct DrivetrainProcessResults {
    float avg_rear_spin;
    float avg_front_spin;
  };
  void process(const DrivetrainProcessParams &p_process_params,
               DrivetrainProcessResults &r_results);
  VehicleDrivetrain();
  float tr11;
  float tr21;
};

VARIANT_ENUM_CAST(VehicleDrivetrain::ShiftRequest);

#endif // VEHICLE_DRIVETRAIN_H
