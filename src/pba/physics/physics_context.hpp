// pba/physics/physics_context.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/physics/forces.hpp"
#include "pba/physics/physics_types.hpp"

#include <array>
#include <memory_resource>

namespace ds_pba
{

struct PhysicsContext
{
    std::vector<RigidBody> bodies;
    std::vector<ExternalForce> external_forces{};

    TimePoint time{};
    Duration time_step{std::chrono::duration<f64>(1.0 / 120.0)};

    std::unordered_map<ContactKey, ContactCacheEntry, ContactKeyHash> contact_cache{};

    std::array<std::byte, k_physics_step_arena_bytes> step_arena_buffer{};
    std::pmr::monotonic_buffer_resource step_arena{
        step_arena_buffer.data(), step_arena_buffer.size()
    };

    struct DebugContact
    {
        ObjectId a_id{k_invalid_id};
        ObjectId b_id{k_invalid_id};
        Position3 p{};
        Direction3 n{k_axis_z};
        f32 penetration{};
        bool allow_warm_start{true};
    };

    std::vector<DebugContact> debug_contacts{};

    f32 debug_total_kinetic_energy{0.0f};

    Duration debug_energy_sample_accum{};
    std::vector<f32> debug_total_kinetic_energy_history{};

    void step();
};

}  // namespace ds_pba
