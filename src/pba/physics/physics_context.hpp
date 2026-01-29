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
    [[nodiscard]] BodyHandle add_body(RigidBody b)
    {
        bodies.push_back(std::move(b));
        return BodyHandle{static_cast<u32>(bodies.size() - 1zu)};
    }

    [[nodiscard]] RigidBody* try_body(BodyHandle h) noexcept
    {
        const auto i = static_cast<usize>(h.index);
        return (i < bodies.size()) ? &bodies[i] : nullptr;
    }

    [[nodiscard]] const RigidBody* try_body(BodyHandle h) const noexcept
    {
        const auto i = static_cast<usize>(h.index);
        return (i < bodies.size()) ? &bodies[i] : nullptr;
    }

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
        EntityId a_id{k_invalid_id};
        EntityId b_id{k_invalid_id};
        Pos3 p{};
        Dir3 n{k_axis_z};
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
