// pba/physics/forces.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/physics/constants.hpp"
#include "pba/physics/physics_types.hpp"
#include "pba/scene/entity_id.hpp"

#include <format>
#include <variant>
//

namespace ds_pba
{

// As per [SoftwareDesign#Ch4] the Visitor pattern should be applied when we have a closed set of
// objects (small number of possible force types) and an open set of operations. The visitor
// pattern can efficiently be introduced using the std::Variant type which avoid runtime overhead.
struct GravityForce
{
    Dir3 accel{k_earth_gravity};
};

struct AttractorForce
{
    Pos3 target{};
    f32 magnitude{10.0f};
    f32 min_radius{0.25f};
};

struct RepulsionForce
{
    Pos3 target{};
    f32 accel_max{15.0f};
    f32 range{5.0f};
    f32 min_radius{0.5f};
};

struct MotorForce
{
    EntityId id{k_invalid_id};
    Dir3 torque{};
};

struct NBodyForce
{
    f32 G{1.0f};
    f32 softening{1e-3f};
};

// Only see a single body at a time / areparallelizable
using SimpleForce = std::variant<GravityForce, AttractorForce, RepulsionForce, MotorForce>;
// Must see all forces / not parallelizable
using ComplexForce = std::variant<NBodyForce>;

// Standard trick to make std::visit pattern more ergonomic
// See > https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

inline auto apply_force(RigidBody& b, const SimpleForce& force) noexcept -> void
{
    if (b.is_static() || b.asleep || b.grabbed)
    {
        return;
    }

    std::visit(
        overloaded{
            [&](const GravityForce& g) noexcept { b.force_accum += g.accel / b.inv_mass; },
            [&](const AttractorForce& a) noexcept
            {
                const auto min_r = std::max(a.min_radius, 1e-6f);
                const auto min_r2 = min_r * min_r;

                const auto d = a.target - b.position;
                const auto d2 = glm::dot(d, d);
                if (d2 <= min_r2)
                {
                    return;
                }

                const auto inv_len = 1.0f / std::sqrt(d2);
                const auto dir = d * inv_len;

                const auto accel = a.magnitude * dir;
                b.force_accum += accel / b.inv_mass;
            },
            [&](const RepulsionForce& r) noexcept
            {
                const auto min_r = std::max(r.min_radius, 1e-6f);
                const auto min_r2 = min_r * min_r;
                const auto range = std::max(r.range, min_r);

                const auto d = b.position - r.target;
                const auto d2 = glm::dot(d, d);
                if (d2 <= min_r2)
                {
                    return;
                }

                const auto dist = std::sqrt(d2);
                if (dist >= range)
                {
                    return;
                }

                const auto dir = d / dist;

                const auto t = (range - dist) / (range - min_r);
                const auto accel_mag = std::clamp(t, 0.0f, 1.0f) * r.accel_max;

                const auto accel = accel_mag * dir;
                b.force_accum += accel / b.inv_mass;
            },
            [&](const MotorForce& m) noexcept
            {
                if (m.id != k_invalid_id && b.id == m.id)
                {
                    b.torque_accum += m.torque;
                }
            },
        },
        force
    );
}

inline auto apply_force(std::span<RigidBody> bodies, const ComplexForce& force) noexcept -> void
{
    std::visit(
        overloaded{
            [&](const NBodyForce& p) noexcept
            {
                const auto n = bodies.size();
                if (n < 2zu)
                {
                    return;
                }

                const auto eps2 = p.softening * p.softening;

                for (usize i{0zu}; i < n; ++i)
                {
                    auto& a = bodies[i];
                    if (a.is_static() || a.asleep || a.grabbed || a.inv_mass <= 0.0f)
                    {
                        continue;
                    }

                    const auto m_a = 1.0f / a.inv_mass;

                    for (usize j{i + 1zu}; j < n; ++j)
                    {
                        auto& b = bodies[j];
                        if (b.is_static() || b.asleep || b.grabbed || b.inv_mass <= 0.0f)
                        {
                            continue;
                        }

                        const auto m_b = 1.0f / b.inv_mass;

                        const auto r = b.position - a.position;
                        const auto r2 = glm::dot(r, r) + eps2;

                        const auto inv_r = 1.0f / std::sqrt(r2);
                        const auto inv_r3 = inv_r * inv_r * inv_r;

                        const auto F = (p.G * m_a * m_b) * r * inv_r3;

                        a.force_accum += F;
                        b.force_accum -= F;
                    }
                }
            },
        },
        force
    );
}
}  // namespace ds_pba
