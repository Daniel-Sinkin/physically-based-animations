// pba/physics/physics_context.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/physics/physics_types.hpp"

#include <array>
#include <memory_resource>

namespace ds_pba
{

struct PhysicsContext
{
    std::vector<RigidBody> bodies;

    TimePoint time{};
    Duration time_step{std::chrono::duration<f64>(1.0 / 120.0)};

    std::unordered_map<ContactKey, ContactCacheEntry, ContactKeyHash> contact_cache{};

    std::array<std::byte, k_physics_step_arena_bytes> step_arena_buffer{};
    std::pmr::monotonic_buffer_resource step_arena{
        step_arena_buffer.data(), step_arena_buffer.size()
    };

    void step();
};

}  // namespace ds_pba
