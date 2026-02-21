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
#include <optional>

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
    struct BoolRef
    {
        u8& value;

        operator bool() const noexcept
        {
            return value != 0u;
        }

        auto operator=(bool rhs) noexcept -> BoolRef&
        {
            value = rhs ? 1u : 0u;
            return *this;
        }
    };

    struct BodyRef
    {
        EntityId& id;
        Dir3& half_extents;
        Pos3& position;
        Dir3& velocity;
        Dir3& force_accum;
        f32& inv_mass;
        Quaternion& orientation;
        Dir3& angular_velocity;
        Dir3& torque_accum;
        glm::mat3& inertia_body;
        glm::mat3& inertia_world;
        glm::mat3& inv_inertia_body;
        glm::mat3& inv_inertia_world;
        BoolRef asleep;
        int& sleep_frames;
        BoolRef grabbed;

        [[nodiscard]] auto is_static() const noexcept -> bool
        {
            return inv_mass <= k_static_mass;
        }
    };

    struct BodyConstRef
    {
        const EntityId& id;
        const Dir3& half_extents;
        const Pos3& position;
        const Dir3& velocity;
        const Dir3& force_accum;
        const f32& inv_mass;
        const Quaternion& orientation;
        const Dir3& angular_velocity;
        const Dir3& torque_accum;
        const glm::mat3& inertia_body;
        const glm::mat3& inertia_world;
        const glm::mat3& inv_inertia_body;
        const glm::mat3& inv_inertia_world;
        bool asleep;
        const int& sleep_frames;
        bool grabbed;

        [[nodiscard]] auto is_static() const noexcept -> bool
        {
            return inv_mass <= k_static_mass;
        }
    };

    [[nodiscard]] auto add_body(RigidBody b) -> BodyHandle
    {
        Expects(bodies_soa.size() < k_max_number_objects);
        bodies_soa.push_back(b);
        return BodyHandle{static_cast<u32>(bodies_soa.size() - 1zu)};
    }

    [[nodiscard]] auto try_body(BodyHandle h) noexcept -> std::optional<BodyRef>;
    [[nodiscard]] auto try_body(BodyHandle h) const noexcept -> std::optional<BodyConstRef>;
    [[nodiscard]] auto body(usize i) noexcept -> BodyRef;
    [[nodiscard]] auto body(usize i) const noexcept -> BodyConstRef;
    [[nodiscard]] auto find_body_index(EntityId id) const noexcept -> std::optional<usize>;
    [[nodiscard]] auto body_count() const noexcept -> usize;

    std::vector<SimpleForce> simple_forces{};
    std::vector<ComplexForce> complex_forces{};

    RigidBodySOA bodies_soa{k_max_number_objects};

    TimePoint time{};
    Duration time_step{std::chrono::duration<f64>(1.0 / k_physics_tick_rate_hz)};

    std::unordered_map<ContactKey, ContactCacheEntry, ContactKeyHash> contact_cache{};

    // Per-Physics Step scratch buffer, cheap to clear
    ArenaAllocator contact_arena{k_physics_step_arena_bytes};
    CollisionScratch collision_scratch{};
    bool debug_tracking_enabled{true};

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

    auto set_debug_tracking_enabled(bool enabled) noexcept -> void;
    auto step() -> void;

    auto clear() -> void;
};

}  // namespace ds_pba
