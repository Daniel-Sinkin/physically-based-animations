// pba/physics/solver.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/physics/solver.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
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

inline bool wake_up(RigidBody& b) noexcept
{  // Returns true if we woke the model up
    if (!b.is_static() && b.asleep)
    {
        b.asleep = false;
        b.sleep_frames = 0;
        return true;
    }
    return false;
}

Quaternion integrate_orientation(const Quaternion& q, const Dir3& omega_world, f32 dt) noexcept
{
    const Quaternion wq{0.0f, omega_world.x, omega_world.y, omega_world.z};
    const Quaternion out{q + (0.5f * dt) * (wq * q)};
    return glm::normalize(out);
}

glm::mat3
inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
{
    const glm::mat3 R{glm::mat3_cast(q)};
    return R * inv_inertia_body * glm::transpose(R);
}

void apply_impulse_contact_friction(
    RigidBody& a, RigidBody& b, Contact& contact, Dir3 r_a, Dir3 r_b, Dir3 n
) noexcept
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
    const Dir3 p_a_dot{a.velocity + glm::cross(a.angular_velocity, r_a)};
    const Dir3 p_b_dot{b.velocity + glm::cross(b.angular_velocity, r_b)};
    const Dir3 v_rel_w{p_a_dot - p_b_dot};

    // Projection onto the tangent
    // v_rel.n
    const auto vrel_dot_n = glm::dot(v_rel_w, n);
    // v_rel - (v_rel.n) * n
    const Dir3 v_t{v_rel_w - vrel_dot_n * n};
    // Equation (21) would be C_{u1}' = v_rel_w . u1
    // Equation (22) would be C_{u2}' = v_rel_w . u2

    // Model friction as a force acting in the opposite direction of "slip"
    const f32 vt2{glm::dot(v_t, v_t)};
    Dir3 t_hat{};
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
    const Dir3 r_a_cross_t{glm::cross(r_a, t_hat)};
    const Dir3 r_b_cross_t{glm::cross(r_b, t_hat)};

    // Change in angular velocity is given by
    //
    // Delta omega = I^-1(r x J)
    //
    const Dir3 invI_r_axt{a.inv_inertia_world * r_a_cross_t};
    const Dir3 invI_r_bxt{b.inv_inertia_world * r_b_cross_t};

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

    const Dir3 friction_impulse{delta_lambda_t * t_hat};

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

void apply_impulse_contact(
    RigidBody& a, RigidBody& b, Contact& contact, f32 restitution, f32 dt_s
) noexcept
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
    f32 eps{restitution};
    // (...) and must satisfy 0 <= eps <= 1. If eps = 1 then v_rel^+ = -v_rel^-,
    // and the collision is perfectly 'bouncy'; in particular, no kinetic energy is lost.
    // At the other end of the spectrum, eps = 0 results in v_rel^+ = 0, and a maxium
    // of kinetic energy is lost.

    // n^(t_0) the unit surface normal. Points from b to a
    const Dir3 n{contact.n};

    // Contact Point
    const Pos3 p{contact.p};

    // Center of mass
    const Pos3 x_a{a.position};
    const Pos3 x_b{b.position};

    // (Linear) Velocity
    const Dir3 v_a{a.velocity};
    const Dir3 v_b{b.velocity};

    // Displacement from center of mass
    const Dir3 r_a{p - x_a};
    const Dir3 r_b{p - x_b};

    // Angular velocities are denoted by omega_a and omega_b respectively
    // (8-1) p_a'(t_0) = v_a(t_0) + omega_a(t_0) x (p_a(t_0) - x_a(t_0))
    const Dir3 p_a_dot{v_a + glm::cross(a.angular_velocity, r_a)};
    // (8-2) p_b'(t_0) = v_b(t_0) + omega_b(t_0) x (p_b(t_0) - x_b(t_0))
    const Dir3 p_b_dot{v_b + glm::cross(b.angular_velocity, r_b)};

    // (8-3) v_rel = n^ . (p_a'(t_0) - p_b'(t_0)) // Normal Relative Velocity
    const f32 v_rel = glm::dot(n, p_a_dot - p_b_dot);

    // Impulse is given by J = j * n^ where we can derive j based on constraints.
    // Will omit the derivation, can express the v_a^+ and v_a^- as well as omega_a^+ and
    // omega_a^- conditions both via coefficient of restitution as well as the impulse identity
    // and solve for the magnitude j

    // Angular impulse direction
    const Dir3 r_a_cross_n{glm::cross(r_a, n)};
    const Dir3 r_b_cross_n{glm::cross(r_b, n)};
    const Dir3 invI_r_axn{a.inv_inertia_world * r_a_cross_n};
    const Dir3 invI_r_bxn{b.inv_inertia_world * r_b_cross_n};

    // (8-18) Denominator
    // "Effective Mass" denominator; in the literature often denoted by k
    const f32 k_a{a.inv_mass + glm::dot(n, glm::cross(invI_r_axn, r_a))};
    const f32 k_b{b.inv_mass + glm::dot(n, glm::cross(invI_r_bxn, r_b))};
    const f32 effective_mass{k_a + k_b};
    if (effective_mass <= 1e-12f)
    {
        return;
    }
    if (-v_rel < k_restitution_threshold)
    {
        eps = 0.0f;
    }

    // delta_lambda_n is accumulation of what is j (impulse magnitude) in the paper
    const f32 old_lambda_n{contact.lambda_n};

    constexpr f32 k_max_bias_speed{2.0f};
    const f32 effective_pen{std::max(0.0f, contact.penetration - k_pen_tolerance)};
    const f32 bias_raw{-(k_pen_correction_frag / dt_s) * effective_pen};
    const f32 bias{std::clamp(bias_raw, -k_max_bias_speed, 0.0f)};
    f32 delta_lambda_n{-(((1.0f + eps) * v_rel) + bias) / effective_mass};

    const f32 new_lambda_n{std::max(old_lambda_n + delta_lambda_n, 0.0f)};
    delta_lambda_n = new_lambda_n - old_lambda_n;
    contact.lambda_n = new_lambda_n;
    if (delta_lambda_n != 0.0f)
    {
        const Dir3 impulse{delta_lambda_n * n};

        // Equation (8-5) Delta v = J / M
        // Equation (8-6) tau_impulse = (p - x(t)) x J
        // "The change in angular velocity is simply I^{-1}(t_0)\tau_{impulse}"
        if (!a.is_static())
        {
            a.velocity += impulse * a.inv_mass;
            const Dir3 tau_a_impulse{glm::cross(r_a, impulse)};
            a.angular_velocity += a.inv_inertia_world * tau_a_impulse;
        }
        if (!b.is_static())
        {
            // Recall n^(t_0) points from b to a so we have to reverse impulse direction
            b.velocity -= impulse * b.inv_mass;
            const Dir3 tau_b_impulse{glm::cross(r_b, impulse)};
            b.angular_velocity -= b.inv_inertia_world * tau_b_impulse;
        }
    }
    apply_impulse_contact_friction(a, b, contact, r_a, r_b, n);
}

void positional_correction_contacts(
    std::vector<RigidBody>& bodies, const Contacts& contacts
) noexcept
{
    for (const auto& contact : contacts)
    {
        auto& a = bodies[contact.a_idx];
        auto& b = bodies[contact.b_idx];

        if (a.is_static() && b.is_static())
        {
            continue;
        }
        const f32 pen{contact.penetration};
        if (pen <= k_pen_tolerance)
        {
            continue;
        }

        const Dir3 n{contact.n};

        const auto inv_mass_a = a.inv_mass;
        const auto inv_mass_b = b.inv_mass;
        const auto inv_mass_sum = inv_mass_a + inv_mass_b;

        if (inv_mass_sum <= 1e-12f)
        {
            continue;
        }

        auto corr_mag = k_pen_percent * (pen - k_pen_tolerance);
        corr_mag = std::clamp(corr_mag, 0.0f, k_pen_max_correction);
        const Dir3 correction{(corr_mag / inv_mass_sum) * n};

        if (!a.is_static())
        {
            a.position += correction * inv_mass_a;
        }
        if (!b.is_static())
        {
            b.position -= correction * inv_mass_b;
        }
    }
}

}  // namespace

void update_inv_inertia_world(std::vector<RigidBody>& bodies) noexcept
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

void integrate_forces(std::vector<RigidBody>& bodies, f32 dt_s) noexcept
{
    for (auto& b : bodies)
    {
        if (b.is_static() || b.asleep)
        {
            continue;
        }
        // (linear) velocity' = F / m
        const Dir3 a{b.force_accum * b.inv_mass};
        b.velocity += a * dt_s;

        // omega' = angular velocity' = I^-1 * torque
        const Dir3 alpha{b.inv_inertia_world * b.torque_accum};
        b.angular_velocity += alpha * dt_s;
    }
}

void integrate_velocities(std::vector<RigidBody>& bodies, f32 dt_s) noexcept
{
    for (auto& b : bodies)
    {
        if (b.is_static() || b.asleep)
        {
            continue;
        }

        b.position += b.velocity * dt_s;
        b.orientation = integrate_orientation(b.orientation, b.angular_velocity, dt_s);

        b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
    }
}

void warm_start_contact(std::vector<RigidBody>& bodies, Contact& contact) noexcept
{

    auto& a = bodies[contact.a_idx];
    auto& b = bodies[contact.b_idx];

    if (a.is_static() && b.is_static())
    {
        return;
    }
    if (a.asleep || b.asleep)
    {
        return;
    }

    const Dir3 n{contact.n};
    const Dir3 r_a{contact.p - a.position};
    const Dir3 r_b{contact.p - b.position};

    constexpr auto warmstart_scale = 1.0f;
    contact.lambda_n = std::clamp(contact.lambda_n, 0.0f, 50.0f);
    contact.lambda_t = std::clamp(contact.lambda_t, -50.0f, 50.0f);
    if (contact.lambda_n > 0.0f)
    {
        const Dir3 Jn{warmstart_scale * contact.lambda_n * n};

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
        const Dir3 Jt{warmstart_scale * contact.lambda_t * contact.t_hat};

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

void solve_velocity_constraints(
    std::vector<RigidBody>& bodies, Contacts& contacts, f32 dt_s
) noexcept
{
    for (usize i{0zu}; i < k_solver_iterations; ++i)
    {
        for (auto& contact : contacts)
        {
            RigidBody& a{bodies[contact.a_idx]};
            RigidBody& b{bodies[contact.b_idx]};
            if (a.is_static() && b.is_static())
            {
                continue;
            }

            const auto pen_eff = contact.penetration - k_pen_tolerance;
            bool woke{false};
            if (pen_eff > 0.0f)
            {
                woke |= wake_up(a);
                woke |= wake_up(b);
            }
            else
            {
                const Dir3 n{contact.n};
                const Dir3 ra{contact.p - a.position};
                const Dir3 rb{contact.p - b.position};

                const Dir3 pa_dot{a.velocity + glm::cross(a.angular_velocity, ra)};
                const Dir3 pb_dot{b.velocity + glm::cross(b.angular_velocity, rb)};
                const auto v_rel_n = glm::dot(n, pa_dot - pb_dot);

                if (v_rel_n < -0.05f)
                {
                    woke |= wake_up(a);
                    woke |= wake_up(b);
                }
            }
            if (woke)
            {
                contact.lambda_n = 0.0f;
                contact.lambda_t = 0.0f;
                contact.has_t_hat = false;
            }

            if (a.asleep && b.asleep)
            {
                continue;
            }

            apply_impulse_contact(a, b, contact, k_restitution, dt_s);
        }
    }
}

void solve_position_constraints(std::vector<RigidBody>& bodies, const Contacts& contacts) noexcept
{
    for (usize i{0zu}; i < k_position_iterations; ++i)
    {
        positional_correction_contacts(bodies, contacts);
    }
}

void apply_sleep_and_damping(std::vector<RigidBody>& bodies, f32 dt_s) noexcept
{
    for (auto& b : bodies)
    {
        if (b.is_static() || b.asleep)
        {
            continue;
        }

        const f32 v2{glm::dot(b.velocity, b.velocity)};
        const f32 w2{glm::dot(b.angular_velocity, b.angular_velocity)};

        const bool slow =
            (v2 < k_linear_sleep_speed_threshold * k_linear_sleep_speed_threshold)
            && (w2 < k_angular_sleep_speed_threshold * k_angular_sleep_speed_threshold);

        if (slow)
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

void clear_accumulators(std::vector<RigidBody>& bodies) noexcept
{
    for (auto& b : bodies)
    {
        b.force_accum = Dir3{};
        b.torque_accum = Dir3{};
    }
}

}  // namespace ds_pba
