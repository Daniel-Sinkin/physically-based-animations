// pba/physics/physics_context.hpp
#pragma once

#include "pba/core/arena_allocator.hpp"
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/physics/collision.hpp"
#include "pba/physics/forces.hpp"
#include "pba/physics/physics_types.hpp"

#include <algorithm>
#include <array>

namespace ds_pba
{

struct EnergyHistoryRing
{
    std::array<f32, k_energy_history_len> values{};
    usize write_head{0zu};
    usize size_{0zu};

    auto clear() noexcept -> void
    {
        write_head = 0zu;
        size_ = 0zu;
    }

    auto push(f32 value) noexcept -> void
    {
        values[write_head] = value;
        write_head = (write_head + 1zu) % values.size();
        size_ = std::min(size_ + 1zu, values.size());
    }

    [[nodiscard]] auto size() const noexcept -> usize
    {
        return size_;
    }

    [[nodiscard]] auto empty() const noexcept -> bool
    {
        return size_ == 0zu;
    }

    [[nodiscard]] auto at(usize i) const noexcept -> f32
    {
        Expects(i < size_);
        const auto oldest = (write_head + values.size() - size_) % values.size();
        const auto idx = (oldest + i) % values.size();
        return values[idx];
    }
};

struct PhysicsContext
{
    std::vector<RigidBody> bodies;
    [[nodiscard]] auto add_body(RigidBody b) -> BodyHandle
    {
        bodies.push_back(std::move(b));
        return BodyHandle{static_cast<u32>(bodies.size() - 1zu)};
    }

    [[nodiscard]] auto try_body(BodyHandle h) noexcept -> RigidBody*
    {
        const auto i = static_cast<usize>(h.index);
        return (i < bodies.size()) ? &bodies[i] : nullptr;
    }

    [[nodiscard]] auto try_body(BodyHandle h) const noexcept -> const RigidBody*
    {
        const auto i = static_cast<usize>(h.index);
        return (i < bodies.size()) ? &bodies[i] : nullptr;
    }

    std::vector<SimpleForce> simple_forces{};
    std::vector<ComplexForce> complex_forces{};

    TimePoint time{};
    Duration time_step{std::chrono::duration<f64>(1.0 / 120.0)};

    std::unordered_map<ContactKey, ContactCacheEntry, ContactKeyHash> contact_cache{};

    // Per-Physics Step scratch buffer, cheap to clear
    ArenaAllocator contact_arena{k_physics_step_arena_bytes};
    CollisionScratch collision_scratch{};

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
    CollisionStats debug_collision_stats{};

    f32 debug_total_kinetic_energy{0.0f};

    Duration debug_energy_sample_accum{};
    EnergyHistoryRing debug_total_kinetic_energy_history{};

    auto step() -> void;

    auto clear() -> void;
};

}  // namespace ds_pba
