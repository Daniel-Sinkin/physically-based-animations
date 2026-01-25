// pba/physics/solver.hpp
#pragma once

#include "pba/physics/physics_types.hpp"

#include <vector>

namespace ds_pba
{

void update_inv_inertia_world(std::vector<RigidBody>& bodies) noexcept;

void integrate_forces(std::vector<RigidBody>& bodies, f32 dt_s) noexcept;

void integrate_velocities(std::vector<RigidBody>& bodies, f32 dt_s) noexcept;

void warm_start_contact(std::vector<RigidBody>& bodies, Contact& contact) noexcept;

void solve_velocity_constraints(
    std::vector<RigidBody>& bodies, Contacts& contacts, f32 dt_s
) noexcept;

void solve_position_constraints(std::vector<RigidBody>& bodies, const Contacts& contacts) noexcept;

void apply_sleep_and_damping(std::vector<RigidBody>& bodies, f32 dt_s) noexcept;

void clear_accumulators(std::vector<RigidBody>& bodies) noexcept;

}  // namespace ds_pba
