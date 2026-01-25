// pba/physics/solver.hpp
#pragma once

#include "pba/physics/physics_types.hpp"

#include <vector>

namespace ds_pba
{

void physics_update_inv_inertia_world(std::vector<RigidBody>& bodies) noexcept;

void physics_integrate_forces(std::vector<RigidBody>& bodies, f32 dt_s) noexcept;

void physics_integrate_velocities(std::vector<RigidBody>& bodies, f32 dt_s) noexcept;

void physics_warm_start_contacts(
    std::vector<RigidBody>& bodies, std::vector<Contact>& contacts
) noexcept;

void physics_solve_velocity_constraints(
    std::vector<RigidBody>& bodies, std::vector<Contact>& contacts, f32 dt_s
) noexcept;

void physics_solve_position_constraints(
    std::vector<RigidBody>& bodies, const std::vector<Contact>& contacts
) noexcept;

void physics_apply_sleep_and_damping(std::vector<RigidBody>& bodies, f32 dt_s) noexcept;

void physics_clear_accumulators(std::vector<RigidBody>& bodies) noexcept;

}  // namespace ds_pba
