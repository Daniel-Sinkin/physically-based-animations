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

// Only see a single body at a time / are parallelizable
using SimpleForce = std::variant<GravityForce, AttractorForce, RepulsionForce, MotorForce>;
// Must see all forces / not parallelizable
using ComplexForce = std::variant<NBodyForce>;

// Standard trick to make std::visit pattern more ergonomic
// See for example https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

inline auto apply_force(RigidBodySOA& bodies, usize body_idx, const SimpleForce& force) noexcept
    -> void
{
    if (
        bodies.is_static(body_idx) || bodies.is_asleep(body_idx) || bodies.is_grabbed(body_idx)
    )
    {
        return;
    }

    const auto inv_mass = bodies.inv_masses[body_idx];
    auto& force_accum = bodies.force_accums[body_idx];
    auto& torque_accum = bodies.torque_accums[body_idx];

    std::visit(
        overloaded{
            [&](const GravityForce& g) noexcept { force_accum += g.accel / inv_mass; },
            [&](const AttractorForce& a) noexcept
            {
                const auto min_r = std::max(a.min_radius, 1e-6f);
                const auto min_r2 = min_r * min_r;

                const auto d = a.target - bodies.positions[body_idx];
                const auto d2 = glm::dot(d, d);
                if (d2 <= min_r2)
                {
                    return;
                }

                const auto inv_len = 1.0f / std::sqrt(d2);
                const auto dir = d * inv_len;

                const auto accel = a.magnitude * dir;
                force_accum += accel / inv_mass;
            },
            [&](const RepulsionForce& r) noexcept
            {
                const auto min_r = std::max(r.min_radius, 1e-6f);
                const auto min_r2 = min_r * min_r;
                const auto range = std::max(r.range, min_r);

                const auto d = bodies.positions[body_idx] - r.target;
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
                force_accum += accel / inv_mass;
            },
            [&](const MotorForce& m) noexcept
            {
                if (m.id != k_invalid_id && bodies.ids[body_idx] == m.id)
                {
                    torque_accum += m.torque;
                }
            },
        },
        force
    );
}

inline auto apply_force(RigidBodySOA& bodies, const ComplexForce& force) noexcept -> void
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
                    if (
                        bodies.is_static(i) || bodies.is_asleep(i) || bodies.is_grabbed(i)
                        || bodies.inv_masses[i] <= 0.0f
                    )
                    {
                        continue;
                    }

                    const auto m_a = 1.0f / bodies.inv_masses[i];

                    for (usize j{i + 1zu}; j < n; ++j)
                    {
                        if (
                            bodies.is_static(j) || bodies.is_asleep(j) || bodies.is_grabbed(j)
                            || bodies.inv_masses[j] <= 0.0f
                        )
                        {
                            continue;
                        }

                        const auto m_b = 1.0f / bodies.inv_masses[j];

                        const auto r = bodies.positions[j] - bodies.positions[i];
                        const auto r2 = glm::dot(r, r) + eps2;

                        const auto inv_r = 1.0f / std::sqrt(r2);
                        const auto inv_r3 = inv_r * inv_r * inv_r;

                        const auto F = (p.G * m_a * m_b) * r * inv_r3;

                        bodies.force_accums[i] += F;
                        bodies.force_accums[j] -= F;
                    }
                }
            },
        },
        force
    );
}
}  // namespace ds_pba
