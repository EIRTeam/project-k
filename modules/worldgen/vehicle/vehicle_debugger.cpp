#include "vehicle_debugger.h"
#include "core/config/engine.h"
#include "core/object/class_db.h"
#include "core/string/print_string.h"
#include "imgui/thirdparty/imgui/imgui.h"
#include "main/performance.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/main/node.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "worldgen/vehicle/vehicle.h"

Vector<float> VehicleDebugger::engine_torques;
Vector<float> VehicleDebugger::drive_torques;
Vector<float> VehicleDebugger::gearbox_shaft_speeds;

void VehicleDebugger::_notification(int p_what) {
  switch (p_what) {
  case NOTIFICATION_READY: {
    im.instantiate();
    MeshInstance3D *mi = memnew(MeshInstance3D);
    mi->set_as_top_level(true);
    add_child(mi);
    mi->set_mesh(im);
    Ref<StandardMaterial3D> sm;
    sm.instantiate();
    sm->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
    sm->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
    mi->set_material_override(sm);
  } break;
  case NOTIFICATION_PROCESS: {
    if (!vehicle) {
      return;
    }

    im->clear_surfaces();
    im->surface_begin(Mesh::PRIMITIVE_LINES);
    for (int i = 0; i < HBVehicle::WHEEL_MAX; i++) {
      Vector3 pos = vehicle->wheels[i]->get_global_position();
      Vector3 fv = vehicle->wheels[i]->force_vec;
      Basis b = vehicle->wheels[i]->get_global_transform().basis;

      im->surface_set_color(Color("RED"));
      im->surface_add_vertex(pos);
      im->surface_add_vertex(pos + b.get_column(0) * fv.x);
      im->surface_set_color(Color("BLUE"));
      im->surface_add_vertex(pos);
      im->surface_add_vertex(pos + b.get_column(2) * fv.y);
    }
    im->surface_end();
    if (ImGui::Begin("Vehicle debugger")) {
      String selected_gear = "N";
      const int selected_gear_number = vehicle->drivetrain->get_current_gear();
      if (selected_gear_number == 0) {
        selected_gear = "N";
      } else if (selected_gear_number > 0) {
        selected_gear = vformat("%d", selected_gear_number);
      } else {
        selected_gear = "R";
      }
      ImGui::Text("Selected gear: %s", selected_gear.utf8().get_data());

      const int rpm_limit = vehicle->engine->get_settings()->get_rpm_limit();
      float rpm = vehicle->engine->get_rpm();

      ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
                            (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));

      ImGui::ProgressBar(rpm / (float)rpm_limit, ImVec2(0.0f, 0.0f));
      ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      ImGui::Text("%.0f", rpm);
      ImGui::Text("engine torque: %.2f", vehicle->engine_torque_out);
      engine_torques.push_back(vehicle->engine_torque_out);
      if (engine_torques.size() > 120) {
        engine_torques.remove_at(0);
      }
      drive_torques.push_back(vehicle->get_drive_reaction_torque());
      if (drive_torques.size() > 120) {
        drive_torques.remove_at(0);
      }
      gearbox_shaft_speeds.push_back(vehicle->gearbox_shaft_speed);
      if (gearbox_shaft_speeds.size() > 120) {
        gearbox_shaft_speeds.remove_at(0);
      }
      ImGui::Text("engine_angular_vel %.2f",
                  vehicle->engine->get_angular_velocity());
      ImGui::Text("clutch_reaction_torque %.2f",
                  vehicle->clutch_reaction_torque);
      ImGui::Text("drivetrain_reaction_torque %.2f",
                  vehicle->drivetrain->get_reaction_torque());

      ImGui::PopStyleColor();
      ImGui::PlotLines("Engine torque", engine_torques.ptr(),
                       engine_torques.size(), 0, nullptr, FLT_MAX, FLT_MAX,
                       ImVec2(0, 100));
      ImGui::PlotLines("Drive reaction torque", drive_torques.ptr(),
                       drive_torques.size(), 0, nullptr, FLT_MAX, FLT_MAX,
                       ImVec2(0, 100));
      ImGui::PlotLines("Gearbox shaft speed", gearbox_shaft_speeds.ptr(),
                       gearbox_shaft_speeds.size(), 0, nullptr, FLT_MAX,
                       FLT_MAX, ImVec2(0, 100));
    }
    ImGui::End();
  } break;
  }
}

void VehicleDebugger::_bind_methods() {
  ClassDB::bind_method(D_METHOD("set_vehicle", "vehicle"),
                       &VehicleDebugger::set_vehicle);
}

void VehicleDebugger::set_vehicle(HBVehicle *p_vehicle) {
  vehicle = p_vehicle;
  Performance::get_singleton()->add_custom_monitor(
      SNAME("drive_reaction_torque"),
      callable_mp(vehicle, &HBVehicle::get_drive_reaction_torque),
      Vector<Variant>());
}

VehicleDebugger::VehicleDebugger() { set_process(true); }
