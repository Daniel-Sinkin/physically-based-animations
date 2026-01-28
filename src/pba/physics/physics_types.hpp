// pba/physics/physics_types.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/scene/entity.hpp"
#include "pba/util/hash.hpp"
//
#include <cmath>
#include <print>
//
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ds_pba
{

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

    glm::mat3 inv_inertia_body{0.0f};
    glm::mat3 inv_inertia_world{0.0f};

    bool asleep{false};
    int sleep_frames{0};

    [[nodiscard]] bool is_static() const noexcept
    {
        return inv_mass == k_static_mass;
    }
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

[[nodiscard]] constexpr const char* to_string(ContactValidity v) noexcept
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

    [[nodiscard]] ContactValidity validate() const
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
using Contacts = std::pmr::vector<Contact>;

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
    usize operator()(const ContactKey& k) const noexcept
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
