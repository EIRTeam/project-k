#ifndef VEHICLE_DEBUGGER_H
#define VEHICLE_DEBUGGER_H

#include "core/io/file_access.h"
#include "scene/main/node.h"
#include "scene/resources/immediate_mesh.h"
#include "worldgen/vehicle/vehicle.h"

class VehicleDebugger : public Node {
  GDCLASS(VehicleDebugger, Node);

  HBVehicle *vehicle = nullptr;
  void _notification(int p_what);
  static void _bind_methods();
  Ref<ImmediateMesh> im;
  Ref<FileAccess> fa;

public:
  static Vector<float> engine_torques;
  static Vector<float> drive_torques;
  static Vector<float> gearbox_shaft_speeds;
  static Vector<float> clutch_states;
  static Vector<float> differential_states;
  void set_vehicle(HBVehicle *p_vehicle);
  VehicleDebugger();
};

#endif // VEHICLE_DEBUGGER_H
