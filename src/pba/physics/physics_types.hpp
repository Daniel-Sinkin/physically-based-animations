// pba/physics/physics_types.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/gsl.hpp"
#include "pba/core/math_types.hpp"
#include "pba/scene/entity_id.hpp"
#include "pba/util/hash.hpp"
//
#include <cmath>
#include <print>
#include <span>
//
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace ds_pba
{

struct BodyHandle
{
    u32 index{};
};

inline constexpr f32 k_static_mass{0.0f};

struct RigidBody
{
    EntityId id{k_invalid_id};

    Dir3 half_extents{0.5f, 0.5f, 0.5f};

    Pos3 position{};
    Dir3 velocity{};
    Dir3 force_accum{};
    f32 inv_mass{k_static_mass};

    Quaternion orientation{k_quaternion_identity};
    Dir3 angular_velocity{};
    Dir3 torque_accum{};

    glm::mat3 inertia_body{0.0f};
    glm::mat3 inertia_world{0.0f};
    glm::mat3 inv_inertia_body{0.0f};
    glm::mat3 inv_inertia_world{0.0f};

    bool asleep{false};
    int sleep_frames{0};

    bool grabbed{false};

    [[nodiscard]] static inline auto is_static_inv_mass(f32 inv_mass) noexcept -> bool
    {
        return inv_mass <= k_static_mass;
    }

    [[nodiscard]] bool is_static() const noexcept
    {
        return is_static_inv_mass(inv_mass);
    }
};

struct RigidBodySOA
{
    explicit RigidBodySOA(usize reserve_count = 0zu)
    {
        reserve(reserve_count);
    }

    auto reserve(usize count) -> void
    {
        ids.reserve(count);
        half_extents.reserve(count);
        positions.reserve(count);
        velocities.reserve(count);
        force_accums.reserve(count);
        inv_masses.reserve(count);
        orientations.reserve(count);
        angular_velocities.reserve(count);
        torque_accums.reserve(count);
        inertia_bodies.reserve(count);
        inertia_worlds.reserve(count);
        inv_inertia_bodies.reserve(count);
        inv_inertia_worlds.reserve(count);
        asleep_flags.reserve(count);
        sleep_frame_counts.reserve(count);
        grabbed_flags.reserve(count);
    }

    auto clear() -> void
    {
        ids.clear();
        half_extents.clear();
        positions.clear();
        velocities.clear();
        force_accums.clear();
        inv_masses.clear();
        orientations.clear();
        angular_velocities.clear();
        torque_accums.clear();
        inertia_bodies.clear();
        inertia_worlds.clear();
        inv_inertia_bodies.clear();
        inv_inertia_worlds.clear();
        asleep_flags.clear();
        sleep_frame_counts.clear();
        grabbed_flags.clear();
    }

    auto resize(usize count) -> void
    {
        ids.resize(count);
        half_extents.resize(count);
        positions.resize(count);
        velocities.resize(count);
        force_accums.resize(count);
        inv_masses.resize(count);
        orientations.resize(count);
        angular_velocities.resize(count);
        torque_accums.resize(count);
        inertia_bodies.resize(count);
        inertia_worlds.resize(count);
        inv_inertia_bodies.resize(count);
        inv_inertia_worlds.resize(count);
        asleep_flags.resize(count);
        sleep_frame_counts.resize(count);
        grabbed_flags.resize(count);
    }

    [[nodiscard]] auto size() const noexcept -> usize
    {
        return ids.size();
    }

    [[nodiscard]] auto empty() const noexcept -> bool
    {
        return ids.empty();
    }

    [[nodiscard]] auto is_static(usize i) const noexcept -> bool
    {
        return RigidBody::is_static_inv_mass(inv_masses[i]);
    }

    [[nodiscard]] auto is_asleep(usize i) const noexcept -> bool
    {
        return asleep_flags[i] != 0u;
    }

    [[nodiscard]] auto is_grabbed(usize i) const noexcept -> bool
    {
        return grabbed_flags[i] != 0u;
    }

    auto set_asleep(usize i, bool value) noexcept -> void
    {
        asleep_flags[i] = value ? 1u : 0u;
    }

    auto set_grabbed(usize i, bool value) noexcept -> void
    {
        grabbed_flags[i] = value ? 1u : 0u;
    }

    auto push_back(const RigidBody& b) -> void
    {
        ids.push_back(b.id);
        half_extents.push_back(b.half_extents);
        positions.push_back(b.position);
        velocities.push_back(b.velocity);
        force_accums.push_back(b.force_accum);
        inv_masses.push_back(b.inv_mass);
        orientations.push_back(b.orientation);
        angular_velocities.push_back(b.angular_velocity);
        torque_accums.push_back(b.torque_accum);
        inertia_bodies.push_back(b.inertia_body);
        inertia_worlds.push_back(b.inertia_world);
        inv_inertia_bodies.push_back(b.inv_inertia_body);
        inv_inertia_worlds.push_back(b.inv_inertia_world);
        asleep_flags.push_back(b.asleep ? 1u : 0u);
        sleep_frame_counts.push_back(b.sleep_frames);
        grabbed_flags.push_back(b.grabbed ? 1u : 0u);
    }

    auto sync_from_aos(std::span<const RigidBody> src) -> void
    {
        resize(src.size());
        for (auto i = 0zu; i < src.size(); ++i)
        {
            const auto& b = src[i];
            ids[i] = b.id;
            half_extents[i] = b.half_extents;
            positions[i] = b.position;
            velocities[i] = b.velocity;
            force_accums[i] = b.force_accum;
            inv_masses[i] = b.inv_mass;
            orientations[i] = b.orientation;
            angular_velocities[i] = b.angular_velocity;
            torque_accums[i] = b.torque_accum;
            inertia_bodies[i] = b.inertia_body;
            inertia_worlds[i] = b.inertia_world;
            inv_inertia_bodies[i] = b.inv_inertia_body;
            inv_inertia_worlds[i] = b.inv_inertia_world;
            asleep_flags[i] = b.asleep ? 1u : 0u;
            sleep_frame_counts[i] = b.sleep_frames;
            grabbed_flags[i] = b.grabbed ? 1u : 0u;
        }
    }

    auto sync_to_aos(std::span<RigidBody> dst) const -> void
    {
        Expects(dst.size() == size());
        for (auto i = 0zu; i < size(); ++i)
        {
            dst[i].id = ids[i];
            dst[i].half_extents = half_extents[i];
            dst[i].position = positions[i];
            dst[i].velocity = velocities[i];
            dst[i].force_accum = force_accums[i];
            dst[i].inv_mass = inv_masses[i];
            dst[i].orientation = orientations[i];
            dst[i].angular_velocity = angular_velocities[i];
            dst[i].torque_accum = torque_accums[i];
            dst[i].inertia_body = inertia_bodies[i];
            dst[i].inertia_world = inertia_worlds[i];
            dst[i].inv_inertia_body = inv_inertia_bodies[i];
            dst[i].inv_inertia_world = inv_inertia_worlds[i];
            dst[i].asleep = asleep_flags[i] != 0u;
            dst[i].sleep_frames = sleep_frame_counts[i];
            dst[i].grabbed = grabbed_flags[i] != 0u;
        }
    }

    std::vector<EntityId> ids{};

    std::vector<Dir3> half_extents{};

    std::vector<Pos3> positions{};
    std::vector<Dir3> velocities{};
    std::vector<Dir3> force_accums{};
    std::vector<f32> inv_masses{};

    std::vector<Quaternion> orientations{};
    std::vector<Dir3> angular_velocities{};
    std::vector<Dir3> torque_accums{};

    std::vector<glm::mat3> inertia_bodies{};
    std::vector<glm::mat3> inertia_worlds{};
    std::vector<glm::mat3> inv_inertia_bodies{};
    std::vector<glm::mat3> inv_inertia_worlds{};

    std::vector<u8> asleep_flags{};
    std::vector<int> sleep_frame_counts{};
    std::vector<u8> grabbed_flags{};
};

enum class ContactValidity : u8
{
    Ok = 0,

    InvalidAId,
    InvalidBId,

    NonFinitePoint,
    NonFiniteNormal,

    NonUnitNormal,
    NegativePenetration,
    NonFinitePenetration,
};

[[nodiscard]] constexpr auto to_string(ContactValidity v) noexcept -> czstring
{
    switch (v)
    {
        case ContactValidity::Ok:
            return "Ok";
        case ContactValidity::InvalidAId:
            return "InvalidAId";
        case ContactValidity::InvalidBId:
            return "InvalidBId";
        case ContactValidity::NonFinitePoint:
            return "NonFinitePoint";
        case ContactValidity::NonFiniteNormal:
            return "NonFiniteNormal";
        case ContactValidity::NonUnitNormal:
            return "NonUnitNormal";
        case ContactValidity::NegativePenetration:
            return "NegativePenetration";
        case ContactValidity::NonFinitePenetration:
            return "NonFinitePenetration";
    }
    return "Unknown";
}

struct Contact
{
    usize a_idx{k_invalid_idx};
    usize b_idx{k_invalid_idx};

    Pos3 p{};           // contact point (world)
    Dir3 n{k_axis_z};   // unit normal (world), direction is b -> a
    f32 penetration{};  // >= 0
    f32 lambda_n{};     // accumulated normal impulse
    f32 lambda_t{};     // accumulated friction impulse

    Dir3 t_hat{};           // tangent direction
    bool has_t_hat{false};  // do we have a cached tangent direction?
    bool allow_warm_start{true};

    [[nodiscard]] auto validate() const -> ContactValidity
    {
        const auto finite_f32 = [](f32 x) noexcept -> bool
        { return std::isfinite(static_cast<f64>(x)); };

        const auto finite_v3 = [&](const glm::vec3& v) noexcept -> bool
        { return finite_f32(v.x) && finite_f32(v.y) && finite_f32(v.z); };

        if (a_idx == k_invalid_idx)
        {
            std::println(
                stderr,
                "Contact invalid: {} (a_idx is k_invalid_idx)",
                to_string(ContactValidity::InvalidAId)
            );
            return ContactValidity::InvalidAId;
        }
        if (b_idx == k_invalid_idx)
        {
            std::println(
                stderr,
                "Contact invalid: {} (b_idx is k_invalid_idx)",
                to_string(ContactValidity::InvalidBId)
            );
            return ContactValidity::InvalidBId;
        }

        if (!finite_v3(p))
        {
            std::println(
                stderr,
                "Contact invalid: {} (p=({}, {}, {}))",
                to_string(ContactValidity::NonFinitePoint),
                static_cast<f64>(p.x),
                static_cast<f64>(p.y),
                static_cast<f64>(p.z)
            );
            return ContactValidity::NonFinitePoint;
        }

        if (!finite_v3(n))
        {
            std::println(
                stderr,
                "Contact invalid: {} (n=({}, {}, {}))",
                to_string(ContactValidity::NonFiniteNormal),
                static_cast<f64>(n.x),
                static_cast<f64>(n.y),
                static_cast<f64>(n.z)
            );
            return ContactValidity::NonFiniteNormal;
        }

        if (!finite_f32(penetration))
        {
            std::println(
                stderr,
                "Contact invalid: {} (penetration={})",
                to_string(ContactValidity::NonFinitePenetration),
                static_cast<f64>(penetration)
            );
            return ContactValidity::NonFinitePenetration;
        }

        if (penetration < 0.0f)
        {
            std::println(
                stderr,
                "Contact invalid: {} (penetration={})",
                to_string(ContactValidity::NegativePenetration),
                static_cast<f64>(penetration)
            );
            return ContactValidity::NegativePenetration;
        }

        const auto len2 = glm::dot(n, n);
        if (!finite_f32(len2) || len2 <= 1e-12f)
        {
            std::println(
                stderr,
                "Contact invalid: {} (|n|^2={})",
                to_string(ContactValidity::NonUnitNormal),
                static_cast<f64>(len2)
            );
            return ContactValidity::NonUnitNormal;
        }

        const auto len = std::sqrt(len2);
        const auto err = std::abs(len - 1.0f);
        if (err > 1e-3f)
        {
            std::println(
                stderr,
                "Contact invalid: {} (|n|={}, err={})",
                to_string(ContactValidity::NonUnitNormal),
                static_cast<f64>(len),
                static_cast<f64>(err)
            );
            return ContactValidity::NonUnitNormal;
        }

        return ContactValidity::Ok;
    }
};

struct ContactCacheEntry
{
    f32 lambda_n{};
    f32 lambda_t{};
    Dir3 t_hat{};
    bool has_t_hat{false};
};

struct ContactKey
{
    EntityId a_id{};
    EntityId b_id{};
    i32 px{};
    i32 py{};
    i32 pz{};

    friend bool operator==(const ContactKey&, const ContactKey&) = default;
};

struct ContactKeyHash
{
    auto operator()(const ContactKey& k) const noexcept -> usize
    {
        usize seed{0zu};
        seed = hash_combine_seed(seed, static_cast<usize>(k.a_id));
        seed = hash_combine_seed(seed, static_cast<usize>(k.b_id));
        seed = hash_combine_seed(seed, static_cast<usize>(static_cast<u32>(k.px)));
        seed = hash_combine_seed(seed, static_cast<usize>(static_cast<u32>(k.py)));
        seed = hash_combine_seed(seed, static_cast<usize>(static_cast<u32>(k.pz)));
        return seed;
    }
};

}  // namespace ds_pba
