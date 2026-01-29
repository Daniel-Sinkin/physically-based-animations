// pba/physics/forces.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/physics/constants.hpp"
#include "pba/physics/physics_types.hpp"
#include "pba/scene/entity.hpp"

#include <vector>

namespace ds_pba
{

using ExternalForceFn = void (*)(std::span<RigidBody> bodies, f32 dt_s, void* user) noexcept;

struct ExternalForce
{
    ExternalForceFn fn{};
    void* user{};
};

struct UniformForce
{
    Dir3 accel{k_earth_gravity};
};
inline void apply_uniform_force(std::span<RigidBody> bodies, f32, void* user) noexcept
{
    const auto& uf = *static_cast<const UniformForce*>(user);

    for (auto& b : bodies)
    {
        if (b.is_static() || b.asleep)
        {
            continue;
        }
        b.force_accum += uf.accel / b.inv_mass;
    }
}

struct AttractorForce
{
    const Pos3* target{};
    f32 accel_mag{10.0f};
    f32 min_radius{0.25f};
};

inline void apply_attractor_force(std::span<RigidBody> bodies, f32, void* user) noexcept
{
    const auto& a = *static_cast<const AttractorForce*>(user);
    if (!a.target)
    {
        return;
    }

    const Pos3 target = *a.target;

    const auto min_r = std::max(a.min_radius, 1e-6f);
    const auto min_r2 = min_r * min_r;

    for (auto& b : bodies)
    {
        if (b.is_static() || b.asleep)
        {
            continue;
        }

        const Dir3 d = target - b.position;
        const auto d2 = glm::dot(d, d);
        if (d2 <= min_r2)
        {
            continue;
        }

        const auto inv_len = 1.0f / std::sqrt(d2);
        const Dir3 dir = d * inv_len;
        const Dir3 accel = a.accel_mag * dir;

        b.force_accum += accel / b.inv_mass;
    }
}

struct RepulsionForce
{
    const Pos3* target{};
    f32 accel_max{15.0f};
    f32 range{5.0f};
    f32 min_radius{0.5f};
};
inline void apply_repulsion_force(std::span<RigidBody> bodies, f32, void* user) noexcept
{
    const auto& r = *static_cast<const RepulsionForce*>(user);
    if (!r.target)
    {
        return;
    }

    const auto min_r = std::max(r.min_radius, 1e-6f);
    const auto min_r2 = min_r * min_r;
    const auto range = std::max(r.range, min_r);

    const Pos3 target = *r.target;

    for (auto& b : bodies)
    {
        if (b.is_static() || b.asleep)
        {
            continue;
        }

        const Dir3 d = b.position - target;
        const auto d2 = glm::dot(d, d);
        if (d2 <= min_r2)
        {
            continue;
        }

        const auto dist = std::sqrt(d2);
        if (dist >= range)
        {
            continue;
        }

        const Dir3 dir = d / dist;

        const auto t = (range - dist) / (range - min_r);
        const auto accel_mag = std::clamp(t, 0.0f, 1.0f) * r.accel_max;

        const Dir3 accel = accel_mag * dir;
        b.force_accum += accel / b.inv_mass;
    }
}

struct Motor
{
    EntityId id{};
    Dir3 torque{};
};

inline void apply_motor_torque(std::span<RigidBody> bodies, f32, void* user) noexcept
{
    auto& m = *static_cast<Motor*>(user);
    for (auto& b : bodies)
    {
        if (b.id == m.id && !b.is_static() && !b.asleep)
        {
            b.torque_accum += m.torque;
            return;
        }
    }
}

struct NBodyParams
{
    f32 G{1.0f};  // Gravitational Constant
    f32 softening{1e-3f};
};

inline void apply_nbody_gravity(std::span<RigidBody> bodies, f32, void* user) noexcept
{
    auto& p = *static_cast<NBodyParams*>(user);
    const auto n = bodies.size();
    for (usize i{0zu}; i < n; ++i)
    {
        auto& a = bodies[i];
        if (a.is_static() || a.asleep)
        {
            continue;
        }

        const auto m_a = 1.0f / a.inv_mass;
        for (usize j{0zu}; j < n; ++j)
        {
            if (i == j)
            {
                continue;
            }
            const auto& b = bodies[j];

            const Dir3 r = b.position - a.position;
            const auto r2 = glm::dot(r, r) + p.softening * p.softening;
            const auto inv_r = 1.0f / std::sqrt(r2);
            const auto inv_r3 = inv_r * inv_r * inv_r;

            const auto m_b = b.is_static() ? 0.0f : (1.0f / b.inv_mass);
            a.force_accum += (p.G * m_b) * r * inv_r3;
            a.force_accum += (p.G * m_a * m_b) * r * inv_r3;
        }
    }
}

}  // namespace ds_pba
