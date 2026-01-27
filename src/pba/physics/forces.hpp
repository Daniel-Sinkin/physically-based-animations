// pba/physics/forces.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/physics/constants.hpp"
#include "pba/physics/physics_types.hpp"

#include <vector>

namespace ds_pba
{

using ExternalForceFn = void (*)(std::vector<RigidBody>& bodies, f32 dt_s, void* user) noexcept;

struct ExternalForce
{
    ExternalForceFn fn{};
    void* user{};
};

struct UniformForce
{
    Direction3 accel{k_earth_gravity};
};
inline void apply_uniform_force(std::vector<RigidBody>& bodies, f32, void* user) noexcept
{
    const auto& uf = *static_cast<const UniformForce*>(user);

    for (RigidBody& b : bodies)
    {
        if (b.is_static() || b.asleep)
        {
            continue;
        }
        const f32 m = 1.0f / b.inv_mass;
        b.force_accum += m * uf.accel;
    }
}

struct AttractorForce
{
    const Position3* target{};
    f32 accel_mag{10.0f};
    f32 min_radius{0.25f};
};

inline void apply_attractor_force(std::vector<RigidBody>& bodies, f32, void* user) noexcept
{
    const auto& a = *static_cast<const AttractorForce*>(user);
    if (!a.target)
    {
        return;
    }

    const Position3 target = *a.target;

    const f32 min_r = std::max(a.min_radius, 1e-6f);
    const f32 min_r2 = min_r * min_r;

    for (RigidBody& b : bodies)
    {
        if (b.is_static() || b.asleep)
        {
            continue;
        }

        const Direction3 d = target - b.position;
        const f32 d2 = glm::dot(d, d);
        if (d2 <= min_r2)
        {
            continue;
        }

        const f32 inv_len = 1.0f / std::sqrt(d2);
        const Direction3 dir = d * inv_len;
        const Direction3 accel = a.accel_mag * dir;

        const f32 m = 1.0f / b.inv_mass;
        b.force_accum += m * accel;
    }
}

struct RepulsionForce
{
    const Position3* target{};
    f32 accel_max{15.0f};
    f32 range{5.0f};
    f32 min_radius{0.5f};
};
inline void apply_repulsion_force(std::vector<RigidBody>& bodies, f32, void* user) noexcept
{
    const auto& r = *static_cast<const RepulsionForce*>(user);
    if (!r.target)
    {
        return;
    }

    const f32 min_r = std::max(r.min_radius, 1e-6f);
    const f32 min_r2 = min_r * min_r;
    const f32 range = std::max(r.range, min_r);

    const Position3 target = *r.target;

    for (RigidBody& b : bodies)
    {
        if (b.is_static() || b.asleep)
        {
            continue;
        }

        const Direction3 d = b.position - target;
        const f32 d2 = glm::dot(d, d);
        if (d2 <= min_r2)
        {
            continue;
        }

        const f32 dist = std::sqrt(d2);
        if (dist >= range)
        {
            continue;
        }

        const Direction3 dir = d / dist;

        const f32 t = (range - dist) / (range - min_r);
        const f32 accel_mag = std::clamp(t, 0.0f, 1.0f) * r.accel_max;

        const Direction3 accel = accel_mag * dir;
        const f32 m = 1.0f / b.inv_mass;
        b.force_accum += m * accel;
    }
}

struct Motor
{
    ObjectId id{};
    Direction3 torque{};
};

inline void apply_motor_torque(std::vector<RigidBody>& bodies, f32, void* user) noexcept
{
    auto& m = *static_cast<Motor*>(user);
    for (RigidBody& b : bodies)
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

inline void apply_nbody_gravity(std::vector<RigidBody>& bodies, f32, void* user) noexcept
{
    auto& p = *static_cast<NBodyParams*>(user);
    const usize n = bodies.size();

    for (usize i{0zu}; i < n; ++i)
    {
        RigidBody& a = bodies[i];
        if (a.is_static() || a.asleep)
        {
            continue;
        }

        const f32 m_a = 1.0f / a.inv_mass;
        for (usize j{0zu}; j < n; ++j)
        {
            if (i == j)
            {
                continue;
            }
            const RigidBody& b = bodies[j];

            const Direction3 r = b.position - a.position;
            const f32 r2 = glm::dot(r, r) + p.softening * p.softening;
            const f32 inv_r = 1.0f / std::sqrt(r2);
            const f32 inv_r3 = inv_r * inv_r * inv_r;

            const f32 m_b = b.is_static() ? 0.0f : (1.0f / b.inv_mass);
            a.force_accum += (p.G * m_b) * r * inv_r3;
            a.force_accum += (p.G * m_a * m_b) * r * inv_r3;
        }
    }
}

}  // namespace ds_pba
