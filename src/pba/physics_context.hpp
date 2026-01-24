// pba/physics_context.hpp
#pragma once

#include "pba/constants.hpp"
#include "pba/core_types.hpp"
#include "pba/math_types.hpp"

#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <print>
#include <vector>

namespace ds_pba
{

inline constexpr f32 k_static_mass{0.0f};

struct RigidBody
{
    ObjectId id{k_invalid_id};

    // Half Extents
    Direction3 half_extents{0.5f, 0.5f, 0.5f};

    // Linear
    Position3 position{};
    Direction3 velocity{};
    Direction3 force_accum{};
    f32 inv_mass{k_static_mass};

    // Angular
    Quaternion orientation{1.0f, 0.0f, 0.0f, 0.0f};
    Direction3 angular_velocity{};  // omega (world space)
    Direction3 torque_accum{};

    glm::mat3 inv_inertia_body{0.0f};   // constant
    glm::mat3 inv_inertia_world{0.0f};  // based on orientation

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

    Position3 p{};           // contact point (world)
    Direction3 n{k_axis_z};  // unit normal (world), b -> a
    f32 penetration{};       // >= 0

    [[nodiscard]] ContactValidity validate() const noexcept
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

        // Normal must be unit length (within tolerance).
        const f32 len2{glm::dot(n, n)};
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

        const f32 len{std::sqrt(len2)};
        const f32 err{std::abs(len - 1.0f)};
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

    [[nodiscard]] bool is_valid() const noexcept
    {
        return validate() == ContactValidity::Ok;
    }
};

struct PhysicsContext
{
    std::vector<RigidBody> bodies;

    TimePoint time{};
    Duration time_step{std::chrono::duration<f64>(1.0 / 120.0)};

    void step();
};

}  // namespace ds_pba
