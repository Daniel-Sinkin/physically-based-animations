// pba/physics/solver.hpp
#pragma once

#include "pba/physics/physics_types.hpp"

#include <span>

namespace ds_pba
{
auto update_inv_inertia_world(std::span<RigidBody> bodies) noexcept -> void;

auto integrate_forces(std::span<RigidBody> bodies, f32 dt_s) noexcept -> void;

auto integrate_velocities(std::span<RigidBody> bodies, f32 dt_s) noexcept -> void;

auto warm_start_contact(std::span<RigidBody> bodies, Contact& contact) noexcept -> void;

auto solve_velocity_constraints(
    std::span<RigidBody> bodies, std::span<Contact> contacts, f32 dt_s
) noexcept -> void;

auto solve_position_constraints(std::span<RigidBody> bodies, std::span<Contact> contacts) noexcept
    -> void;

auto apply_sleep_and_damping(std::span<RigidBody> bodies, f32 dt_s) noexcept -> void;

auto clear_accumulators(std::span<RigidBody> bodies) noexcept -> void;
}  // namespace ds_pba
