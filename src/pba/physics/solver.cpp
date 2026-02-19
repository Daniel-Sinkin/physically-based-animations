// pba/physics/solver.cpp
#include "pba/core/arena_allocator.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/physics/solver.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/physics/physics_types.hpp"
//
#include <algorithm>
#include <cassert>
#include <cmath>
//
#include <glm/geometric.hpp>

namespace ds_pba
{
namespace
{

inline auto wake_up(RigidBody& b) noexcept -> bool
{  // Returns true if we woke the model up
    if (!b.is_static() && b.asleep)
    {
        b.asleep = false;
        b.sleep_frames = 0;
        return true;
    }
    return false;
}

auto integrate_orientation(const Quaternion& q, const Dir3& omega_world, f32 dt) noexcept
    -> Quaternion
{
    const auto wq = Quaternion{0.0f, omega_world.x, omega_world.y, omega_world.z};
    return glm::normalize(q + (0.5f * dt) * (wq * q));
}

auto inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
    -> glm::mat3
{
    const auto R = glm::mat3_cast(q);
    return R * inv_inertia_body * glm::transpose(R);
}

auto apply_impulse_contact_friction(
    RigidBody& a, RigidBody& b, Contact& contact, Dir3 r_a, Dir3 r_b, Dir3 n
) noexcept -> void
{
    {
        Expects(!a.is_static() || !b.is_static());
        Expects(!a.asleep || !b.asleep);
    }
    // This section adapted from [Catto05] (4.3) Friction Constraints.
    // The paper explicitly uses a orthonormal tangent basis {u1, u2} with
    // u1 x u2 = n and computes (v_rel.u1) and (v_rel.u2) seperately.
    // To avoid computing these U:={u1, u2} explicitly this code does
    //
    // proj_U(v_rel) = v_rel - (v_rel.n)*n == (v_rel.u1)*u1 + (v_rel.u2)*u2
    //
    // which is an orthonormal projection onto the orthogonal space of the
    // normal direction (i.e. exactly the tangent space).
    const auto p_a_dot = a.velocity + glm::cross(a.angular_velocity, r_a);
    const auto p_b_dot = b.velocity + glm::cross(b.angular_velocity, r_b);
    const auto v_rel_w = p_a_dot - p_b_dot;

    // Projection onto the tangent
    // v_rel.n
    const auto vrel_dot_n = glm::dot(v_rel_w, n);
    // v_rel - (v_rel.n) * n
    const auto v_t = v_rel_w - vrel_dot_n * n;
    // Equation (21) would be C_{u1}' = v_rel_w . u1
    // Equation (22) would be C_{u2}' = v_rel_w . u2

    // Model friction as a force acting in the opposite direction of "slip"
    const auto vt2 = glm::dot(v_t, v_t);
    auto t_hat = Dir3{};
    if (vt2 > 1e-12f)
    {
        t_hat = v_t / std::sqrt(vt2);

        contact.t_hat = t_hat;
        contact.has_t_hat = true;
    }
    else
    {
        if (!contact.has_t_hat)
        {
            return;
        }
        t_hat = contact.t_hat;
    }

    // These are the angular Jacobian terms (r x u) from Equation (23)
    const auto r_a_cross_t = glm::cross(r_a, t_hat);
    const auto r_b_cross_t = glm::cross(r_b, t_hat);

    // Change in angular velocity is given by
    //
    // Delta omega = I^-1(r x J)
    //
    const auto invI_r_axt = a.inv_inertia_world * r_a_cross_t;
    const auto invI_r_bxt = b.inv_inertia_world * r_b_cross_t;

    // Computes effective mass along t_hat, often denoted by just k
    const auto k2a = a.inv_mass + glm::dot(t_hat, glm::cross(invI_r_axt, r_a));
    const auto k2b = b.inv_mass + glm::dot(t_hat, glm::cross(invI_r_bxt, r_b));
    const auto effective_mass = k2a + k2b;
    if (effective_mass <= 1e-12f)
    {
        return;
    }

    const auto old_lambda_t = contact.lambda_t;

    auto delta_lambda_t = -glm::dot(v_rel_w, t_hat) / effective_mass;
    const auto max_jt = k_friction * contact.lambda_n;
    if (max_jt <= 0.0f)
    {
        contact.lambda_t = 0.0f;
        return;
    }

    const auto new_lambda_t = std::clamp(old_lambda_t + delta_lambda_t, -max_jt, +max_jt);
    delta_lambda_t = new_lambda_t - old_lambda_t;
    contact.lambda_t = new_lambda_t;

    const auto friction_impulse = delta_lambda_t * t_hat;

    if (!a.is_static())
    {
        a.velocity += friction_impulse * a.inv_mass;
        a.angular_velocity += a.inv_inertia_world * glm::cross(r_a, friction_impulse);
    }
    if (!b.is_static())
    {
        b.velocity -= friction_impulse * b.inv_mass;
        b.angular_velocity -= b.inv_inertia_world * glm::cross(r_b, friction_impulse);
    }
}

auto apply_impulse_contact(
    RigidBody& a, RigidBody& b, Contact& contact, f32 restitution, f32 dt_s
) noexcept -> void
{
    {
        Expects(!a.is_static() || !b.is_static());
        Expects(!a.asleep || !b.asleep);
        Expects(restitution > 0.0f && restitution <= 1.0f);
        Expects(std::abs(glm::length(contact.n) - 1.0f) < 1e-5f);
    }
    // This is based on [Baraff97]

    // "The quantity eps is called the coefficient of restitution (...)"
    constexpr f32 k_restitution_threshold{2.0f};
    auto eps = restitution;
    // (...) and must satisfy 0 <= eps <= 1. If eps = 1 then v_rel^+ = -v_rel^-,
    // and the collision is perfectly 'bouncy'; in particular, no kinetic energy is lost.
    // At the other end of the spectrum, eps = 0 results in v_rel^+ = 0, and a maxium
    // of kinetic energy is lost.

    // n^(t_0) the unit surface normal. Points from b to a
    const auto n = contact.n;

    // Contact Point
    const auto p = contact.p;

    // Center of mass
    const auto x_a = a.position;
    const auto x_b = b.position;

    // (Linear) Velocity
    const auto v_a = a.velocity;
    const auto v_b = b.velocity;

    // Displacement from center of mass
    const auto r_a = p - x_a;
    const auto r_b = p - x_b;

    // Angular velocities are denoted by omega_a and omega_b respectively
    // (8-1) p_a'(t_0) = v_a(t_0) + omega_a(t_0) x (p_a(t_0) - x_a(t_0))
    const auto p_a_dot = v_a + glm::cross(a.angular_velocity, r_a);
    // (8-2) p_b'(t_0) = v_b(t_0) + omega_b(t_0) x (p_b(t_0) - x_b(t_0))
    const auto p_b_dot = v_b + glm::cross(b.angular_velocity, r_b);

    // (8-3) v_rel = n^ . (p_a'(t_0) - p_b'(t_0)) // Normal Relative Velocity
    const auto v_rel = glm::dot(n, p_a_dot - p_b_dot);

    // Impulse is given by J = j * n^ where we can derive j based on constraints.
    // Will omit the derivation, can express the v_a^+ and v_a^- as well as omega_a^+ and
    // omega_a^- conditions both via coefficient of restitution as well as the impulse identity
    // and solve for the magnitude j

    // Angular impulse direction
    const auto r_a_cross_n = glm::cross(r_a, n);
    const auto r_b_cross_n = glm::cross(r_b, n);
    const auto invI_r_axn = a.inv_inertia_world * r_a_cross_n;
    const auto invI_r_bxn = b.inv_inertia_world * r_b_cross_n;

    // (8-18) Denominator
    // "Effective Mass" denominator; in the literature often denoted by k
    const auto k_a = a.inv_mass + glm::dot(n, glm::cross(invI_r_axn, r_a));
    const auto k_b = b.inv_mass + glm::dot(n, glm::cross(invI_r_bxn, r_b));
    const auto effective_mass = k_a + k_b;
    if (effective_mass <= 1e-12f)
    {
        return;
    }
    if (-v_rel < k_restitution_threshold)
    {
        eps = 0.0f;
    }

    // delta_lambda_n is accumulation of what is j (impulse magnitude) in the paper
    const auto old_lambda_n = contact.lambda_n;

    constexpr f32 k_max_bias_speed{2.0f};
    const auto effective_pen = std::max(0.0f, contact.penetration - k_pen_tolerance);
    const auto bias_raw = -(k_pen_correction_frag / dt_s) * effective_pen;
    const auto bias = std::clamp(bias_raw, -k_max_bias_speed, 0.0f);
    auto delta_lambda_n = -(((1.0f + eps) * v_rel) + bias) / effective_mass;

    const auto new_lambda_n = std::max(old_lambda_n + delta_lambda_n, 0.0f);
    delta_lambda_n = new_lambda_n - old_lambda_n;
    contact.lambda_n = new_lambda_n;
    if (delta_lambda_n != 0.0f)
    {
        const auto impulse = delta_lambda_n * n;

        // Equation (8-5) Delta v = J / M
        // Equation (8-6) tau_impulse = (p - x(t)) x J
        // "The change in angular velocity is simply I^{-1}(t_0)\tau_{impulse}"
        if (!a.is_static())
        {
            a.velocity += impulse * a.inv_mass;
            const auto tau_a_impulse = glm::cross(r_a, impulse);
            a.angular_velocity += a.inv_inertia_world * tau_a_impulse;
        }
        if (!b.is_static())
        {
            // Recall n^(t_0) points from b to a so we have to reverse impulse direction
            b.velocity -= impulse * b.inv_mass;
            const auto tau_b_impulse = glm::cross(r_b, impulse);
            b.angular_velocity -= b.inv_inertia_world * tau_b_impulse;
        }
    }
    apply_impulse_contact_friction(a, b, contact, r_a, r_b, n);
}

}  // namespace

auto update_inv_inertia_world(std::span<RigidBody> bodies) noexcept -> void
{
    for (auto& b : bodies)
    {
        if (b.is_static())
        {
            b.inv_inertia_world = glm::mat3(0.0f);
            continue;
        }
        b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
    }
}

auto integrate_forces(std::span<RigidBody> bodies, f32 dt_s) noexcept -> void
{
    for (auto& b : bodies)
    {
        if (b.is_static() || b.asleep || b.grabbed)
        {
            continue;
        }
        // (linear) velocity' = F / m
        const auto a = b.force_accum * b.inv_mass;
        b.velocity += a * dt_s;

        // omega' = angular velocity' = I^-1 * torque
        const auto alpha = b.inv_inertia_world * b.torque_accum;
        b.angular_velocity += alpha * dt_s;
    }
}

auto integrate_velocities(std::span<RigidBody> bodies, f32 dt_s) noexcept -> void
{
    for (auto& b : bodies)
    {
        if (b.is_static() || b.asleep || b.grabbed)
        {
            continue;
        }

        b.position += b.velocity * dt_s;
        b.orientation = integrate_orientation(b.orientation, b.angular_velocity, dt_s);

        b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
    }
}

auto warm_start_contact(std::span<RigidBody> bodies, Contact& contact) noexcept -> void
{

    auto& a = bodies[contact.a_idx];
    auto& b = bodies[contact.b_idx];

    if (a.is_static() && b.is_static())
    {
        return;
    }
    if (a.asleep || b.asleep || a.grabbed || b.grabbed)
    {
        return;
    }

    const auto normal = contact.n;
    const auto r_a = contact.p - a.position;
    const auto r_b = contact.p - b.position;

    constexpr auto warmstart_scale = 1.0f;
    contact.lambda_n = std::clamp(contact.lambda_n, 0.0f, 50.0f);
    contact.lambda_t = std::clamp(contact.lambda_t, -50.0f, 50.0f);
    if (contact.lambda_n > 0.0f)
    {
        const auto Jn = warmstart_scale * contact.lambda_n * normal;

        if (!a.is_static())
        {
            a.velocity += Jn * a.inv_mass;
            a.angular_velocity += a.inv_inertia_world * glm::cross(r_a, Jn);
        }
        if (!b.is_static())
        {
            b.velocity -= Jn * b.inv_mass;
            b.angular_velocity -= b.inv_inertia_world * glm::cross(r_b, Jn);
        }
    }

    if (contact.lambda_t != 0.0f && contact.has_t_hat)
    {
        const auto Jt = warmstart_scale * contact.lambda_t * contact.t_hat;

        if (!a.is_static())
        {
            a.velocity += Jt * a.inv_mass;
            a.angular_velocity += a.inv_inertia_world * glm::cross(r_a, Jt);
        }
        if (!b.is_static())
        {
            b.velocity -= Jt * b.inv_mass;
            b.angular_velocity -= b.inv_inertia_world * glm::cross(r_b, Jt);
        }
    }
}

[[nodiscard]] inline auto
get_velocity_at_world_point(const RigidBody& b, const Pos3& world_p) noexcept -> Dir3
{
    return b.velocity + glm::cross(b.angular_velocity, world_p - b.position);
}

[[nodiscard]] inline auto get_relative_velocity_at_world_point(
    const RigidBody& a, const RigidBody& b, const Pos3& world_p
) noexcept -> Dir3
{
    return get_velocity_at_world_point(a, world_p) - get_velocity_at_world_point(b, world_p);
}

auto solve_velocity_constraints(
    std::span<RigidBody> bodies, ArenaAllocator& contact_arena, f32 dt_s
) noexcept -> void
{
    for (auto& contact : contact_arena.as_span<Contact>())
    {
        auto& a = bodies[contact.a_idx];
        auto& b = bodies[contact.b_idx];

        auto can_wake_up = [](RigidBody& rb) -> bool { return rb.is_static() || !rb.asleep; };
        if (can_wake_up(a) && can_wake_up(b))
        {
            continue;
        }

        const auto should_wake = [&]() -> bool
        {
            if (contact.penetration > k_pen_tolerance)
            {
                return true;
            }
            const auto v_rel = get_relative_velocity_at_world_point(a, b, contact.p);
            return glm::dot(contact.n, v_rel) < -0.05f;
        }();
        if (should_wake && (wake_up(a) || wake_up(b)))
        {
            contact.lambda_n = 0.0f;
            contact.lambda_t = 0.0f;
            contact.has_t_hat = false;
        }
    }

    for (auto i = 0; i < k_solver_iterations; ++i)
    {
        for (auto& contact : contact_arena.as_span<Contact>())
        {
            auto& a{bodies[contact.a_idx]};
            auto& b{bodies[contact.b_idx]};
            if (a.is_static() && b.is_static())
            {
                continue;
            }
            if (a.grabbed || b.grabbed)
            {
                contact.lambda_n = 0.0f;
                contact.lambda_t = 0.0f;
                contact.has_t_hat = false;
                continue;
            }
            if (a.asleep && b.asleep)
            {
                continue;
            }
            apply_impulse_contact(a, b, contact, k_restitution, dt_s);
        }
    }
}

auto solve_position_constraints(std::span<RigidBody> bodies, ArenaAllocator& contact_arena) noexcept
    -> void
{
    for (auto i = 0; i < k_position_iterations; ++i)
    {
        for (auto& contact : contact_arena.as_span<Contact>())
        {
            auto& a = bodies[contact.a_idx];
            auto& b = bodies[contact.b_idx];

            if ((a.is_static() && b.is_static()) || contact.penetration <= k_pen_tolerance)
            {
                continue;
            }

            const auto inv_mass_sum = a.inv_mass + b.inv_mass;
            if (inv_mass_sum <= 1e-12f)
            {
                continue;
            }

            const auto correction = [&]()
            {
                const auto pen_excess = contact.penetration - k_pen_tolerance;
                const auto mag = std::clamp(k_pen_percent * pen_excess, 0.0f, k_pen_max_correction);
                return (mag / inv_mass_sum) * contact.n;
            }();
            if (!a.is_static())
            {
                a.position += correction * a.inv_mass;
            }
            if (!b.is_static())
            {
                b.position -= correction * b.inv_mass;
            }
        }
    }
}

auto apply_sleep_and_damping(std::span<RigidBody> bodies, f32 dt_s) noexcept -> void
{
    for (auto& b : bodies)
    {
        if (b.grabbed)
        {
            b.velocity = Dir3{};
            b.angular_velocity = Dir3{};
            continue;
        }
        if (b.is_static() || b.asleep)
        {
            continue;
        }

        const auto v2 = glm::dot(b.velocity, b.velocity);
        const auto w2 = glm::dot(b.angular_velocity, b.angular_velocity);

        auto v_lim = k_linear_sleep_speed_threshold;
        auto w_lim = k_angular_sleep_speed_threshold;
        const auto velo_slow = v2 < v_lim * v_lim;
        const auto angular_velo_slow = w2 < w_lim * w_lim;

        if (velo_slow && angular_velo_slow)
        {
            ++b.sleep_frames;
            if (b.sleep_frames > 60)
            {
                b.asleep = true;
                b.velocity = Dir3{};
                b.angular_velocity = Dir3{};
                b.force_accum = Dir3{};
                b.torque_accum = Dir3{};
            }
        }
        else
        {
            b.sleep_frames = 0;
        }
        b.velocity *= std::exp(-k_linear_damping * dt_s);
        b.angular_velocity *= std::exp(-k_angular_damping * dt_s);
    }
}

auto clear_accumulators(std::span<RigidBody> bodies) noexcept -> void
{
    for (auto& b : bodies)
    {
        b.force_accum = Dir3{};
        b.torque_accum = Dir3{};
    }
}

}  // namespace ds_pba
