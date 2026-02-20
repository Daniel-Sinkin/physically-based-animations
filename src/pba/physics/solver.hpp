// pba/physics/solver.hpp
#pragma once

#include "pba/core/arena_allocator.hpp"
#include "pba/physics/physics_types.hpp"

namespace ds_pba
{
auto update_inv_inertia_world(RigidBodySOA& bodies) noexcept -> void;

auto integrate_forces(RigidBodySOA& bodies, f32 dt_s) noexcept -> void;

auto integrate_velocities(RigidBodySOA& bodies, f32 dt_s) noexcept -> void;

auto warm_start_contact(RigidBodySOA& bodies, Contact& contact) noexcept -> void;

auto solve_velocity_constraints(RigidBodySOA& bodies, ArenaAllocator& contacts, f32 dt_s) noexcept
    -> void;

auto solve_position_constraints(RigidBodySOA& bodies, ArenaAllocator& contacts) noexcept -> void;

auto apply_sleep_and_damping(RigidBodySOA& bodies, f32 dt_s) noexcept -> void;

auto clear_accumulators(RigidBodySOA& bodies) noexcept -> void;
}  // namespace ds_pba
