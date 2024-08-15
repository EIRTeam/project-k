#include "vehicle_suspension_corner.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/templates/hash_set.h"
#include "scene/3d/physics/rigid_body_3d.h"
#include "servers/physics_server_3d.h"
#include "worldgen/vehicle/vehicle.h"

bool VehicleSuspensionCorner::process_collision(Vector3 &r_position,
                                                Vector3 &r_normal) const {
  const float travel = suspension_settings->get_spring_travel();
  const float tire_radius =
      suspension_settings->get_tire_settings()->get_tire_radius();
  const float check_distance = travel + tire_radius;

  PhysicsDirectSpaceState3D *dss = get_world_3d()->get_direct_space_state();

  HBVehicle *parent = Object::cast_to<HBVehicle>(get_parent());
  HashSet<RID> exclude;

  if (parent) {
    exclude.insert(parent->get_rid());
  }

  PhysicsDirectSpaceState3D::RayResult result;
  bool collided = dss->intersect_ray(
      {.from = get_global_position(),
       .to = get_global_position() +
             get_global_basis().xform(Vector3(0.0, -check_distance, 0.0)),
       .exclude = exclude},
      result);

  if (collided) {
    r_position = result.position;
    r_normal = result.normal;
  }

  return collided;
}

void VehicleSuspensionCorner::_bind_methods() {
  ClassDB::bind_method(D_METHOD("get_current_spring_length"),
                       &VehicleSuspensionCorner::get_current_spring_length);
  ClassDB::bind_method(D_METHOD("get_spin"),
                       &VehicleSuspensionCorner::get_spin);
  ClassDB::bind_method(D_METHOD("set_spin"),
                       &VehicleSuspensionCorner::set_spin);
  ClassDB::bind_method(
      D_METHOD("apply_forces", "car", "opposite_comp", "delta"),
      &VehicleSuspensionCorner::apply_forces);
  ClassDB::bind_method(D_METHOD("apply_torque", "drive_torque", "brake_torque",
                                "drive_inertia", "delta"),
                       &VehicleSuspensionCorner::apply_torque);
  ClassDB::bind_method(D_METHOD("steer"), &VehicleSuspensionCorner::steer);
  ClassDB::bind_method(D_METHOD("get_reaction_torque"),
                       &VehicleSuspensionCorner::get_reaction_torque);
  ClassDB::bind_method(
      D_METHOD("set_suspension_settings", "suspension_settings"),
      &VehicleSuspensionCorner::set_suspension_settings);
  ClassDB::bind_method(D_METHOD("get_suspension_settings"),
                       &VehicleSuspensionCorner::get_suspension_settings);
  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "suspension_settings",
                            PROPERTY_HINT_RESOURCE_TYPE,
                            "VehicleSuspensionCornerSettings"),
               "set_suspension_settings", "get_suspension_settings");
  ClassDB::bind_method(D_METHOD("get_wheel_inertia"),
                       &VehicleSuspensionCorner::get_wheel_inertia);
}

float VehicleSuspensionCorner::apply_forces(RigidBody3D *p_car,
                                            float p_opposite_comp,
                                            float p_delta) {
  Vector3 collision_point;
  Vector3 collision_normal;
  const bool colliding = process_collision(collision_point, collision_normal);

  force_vec = Vector3();

  Vector3 local_vel = get_global_transform().basis.xform_inv(
      (get_global_transform().origin - prev_pos) / p_delta);
  float z_vel = -local_vel.z;
  Vector2 planar_vect = Vector2(local_vel.x, local_vel.y).normalized();
  prev_pos = get_global_transform().origin;

  // TODO: add different surface support
  float surface_mu = 0.85;

  const float spring_length = suspension_settings->get_spring_travel();

  if (colliding) {
    current_spring_length =
        collision_point.distance_to(get_global_transform().origin) -
        suspension_settings->get_tire_settings()->get_tire_radius();
  } else {
    current_spring_length = spring_length;
  }

  // Calculate the spring load in mm (absolut)
  float spring_load_mm = (spring_length - current_spring_length) * 1000.0f;

  // Calculate spring movement in mm per seconds
  float spring_speed_mm_per_seconds =
      (spring_load_mm - prev_spring_load_mm) / p_delta;
  prev_spring_load_mm = spring_load_mm;

  // Calculate the force of the spring in N (mm * N/mm  equals m * kN/m)
  float spring_load_newton =
      spring_load_mm * suspension_settings->get_spring_rate();

  // Calculate the damping force in N and add it to spring_load_newton
  if (spring_speed_mm_per_seconds >= 0.0f) {
    spring_load_newton +=
        spring_speed_mm_per_seconds * suspension_settings->get_bump(); // bump
  } else {
    spring_load_newton += spring_speed_mm_per_seconds *
                          suspension_settings->get_rebound(); // rebound
  }
  float y_force = spring_load_newton;
  y_force = MAX(0.0, y_force);

  // Slip
  Vector2 slip_vec;
  slip_vec.x =
      Math::asin(CLAMP(-planar_vect.x, -1.0f, 1.0f)); // X slip is lateral slip
  slip_vec.y = 0.0f; // Y slip is the longitudinal Z slip

  const float tire_radius =
      suspension_settings->get_tire_settings()->get_tire_radius();
  if (colliding) {
    if (!Math::is_zero_approx(z_vel)) {
      slip_vec.y = (z_vel - spin * tire_radius) / Math::abs(z_vel);
    } else {
      slip_vec.y = (z_vel - spin * tire_radius) / Math::abs(z_vel + 0.0000001f);
    }
    if (spring_load_mm != 0.0f) {
      y_force += suspension_settings->get_anti_roll() *
                 (spring_load_mm - p_opposite_comp);
    }

    force_vec = suspension_settings->get_tire_settings()->update_forces(
        slip_vec, y_force, surface_mu);

    Vector3 contact = collision_point - p_car->get_global_transform().origin;

    p_car->apply_force(collision_normal * y_force, contact);
    p_car->apply_force(get_global_transform().basis.get_column(0) * force_vec.x,
                       contact);
    p_car->apply_force(get_global_transform().basis.get_column(2) * force_vec.y,
                       contact);

    return spring_load_mm;
  }

  const float wheel_inertia = get_wheel_inertia();

  spin -= SIGN(spin) * p_delta * 2.0 /
          wheel_inertia; // stop undriven wheels from spinning endlessly
  return 0.0f;
}

float VehicleSuspensionCorner::apply_torque(float p_drive_torque,
                                            float p_brake_torque,
                                            float p_drive_inertia,
                                            float p_delta) {
  const float tire_radius =
      suspension_settings->get_tire_settings()->get_tire_radius();

  float prev_spin = spin;
  float net_torque = force_vec.y * tire_radius;
  net_torque += p_drive_torque;

  const float wheel_inertia = get_wheel_inertia();

  if (Math::abs(spin) < 5.0f && p_brake_torque > Math::abs(net_torque)) {
    spin = 0.0f;
  } else {
    // TODO
    const float ROLLING_RESISTANCE = 0.0f;
    net_torque -= (p_brake_torque + ROLLING_RESISTANCE) * SIGN(spin);
    spin += p_delta * net_torque / (wheel_inertia + p_drive_inertia);
  }

  if (p_drive_torque * p_delta == 0.0f) {
    return 0.5f;
  }

  return (spin - prev_spin) * (wheel_inertia + p_drive_inertia) /
         (p_drive_torque * p_delta);
}

void VehicleSuspensionCorner::steer(float p_input, float p_max_steer,
                                    float p_ackermann) {
  Vector3 rotation = get_rotation();
  rotation.y =
      p_max_steer *
      (p_input + (1 - Math::cos(p_input * 0.5 * Math_PI)) * p_ackermann);
  set_rotation(rotation);
}

float VehicleSuspensionCorner::get_reaction_torque() const {
  const float tire_radius =
      suspension_settings->get_tire_settings()->get_tire_radius();
  return force_vec.y * tire_radius;
}

float VehicleSuspensionCorner::get_rolling_resistance() const {
  return rolling_resistance;
}

void VehicleSuspensionCorner::set_rolling_resistance(
    float p_rolling_resistance) {
  rolling_resistance = p_rolling_resistance;
}

float VehicleSuspensionCorner::get_wheel_inertia() const {
  const float tire_radius =
      suspension_settings->get_tire_settings()->get_tire_radius();
  return 0.5f * suspension_settings->get_wheel_mass() * tire_radius *
         tire_radius;
}
