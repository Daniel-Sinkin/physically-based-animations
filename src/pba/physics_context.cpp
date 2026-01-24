// pba/physics_context.cpp
#include "glm/geometric.hpp"
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/physics_context.hpp"
//

namespace ds_pba
{
namespace
{

struct Contact
{
    usize body_idx{};
    Position3 p{0.0f, 0.0f, 0.0f};
    Direction3 n{0.0f, 0.0f, 1.0f};
    f32 penetration{0.0f};
};

[[nodiscard]] f32 dt_f32(const Duration& dt) noexcept
{
    return static_cast<f32>(dt.count());
}

[[nodiscard]] Quaternion
integrate_orientation(const Quaternion& q, const Direction3& omega_world, f32 dt) noexcept
{
    const Quaternion wq{0.0f, omega_world.x, omega_world.y, omega_world.z};
    Quaternion out = q + (0.5f * dt) * (wq * q);
    return glm::normalize(out);
}

[[nodiscard]] glm::mat3
inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
{
    const glm::mat3 R = glm::mat3_cast(q);
    return R * inv_inertia_body * glm::transpose(R);
}

[[nodiscard]] std::array<Position3, 8> box_world_corners(const RigidBody& b) noexcept
{
    const glm::mat3 R = glm::mat3_cast(b.orientation);
    const Direction3 he = b.half_extents;

    const Direction3 ex = R * Direction3{he.x, 0.0f, 0.0f};
    const Direction3 ey = R * Direction3{0.0f, he.y, 0.0f};
    const Direction3 ez = R * Direction3{0.0f, 0.0f, he.z};

    std::array<Position3, 8> c{};

    usize i{0zu};
    for (int sx{-1}; sx <= 1; sx += 2)
    {
        for (int sy{-1}; sy <= 1; sy += 2)
        {
            for (int sz{-1}; sz <= 1; sz += 2)
            {
                const f32 sxf = static_cast<f32>(sx);
                const f32 syf = static_cast<f32>(sy);
                const f32 szf = static_cast<f32>(sz);

                c[i++] = b.position + sxf * ex + syf * ey + szf * ez;
            }
        }
    }

    return c;
}

void generate_ground_contacts(const std::vector<RigidBody>& bodies, std::vector<Contact>& out)
{
    out.clear();
    out.reserve(bodies.size() * 2zu);

    constexpr Direction3 n{0.0f, 0.0f, 1.0f};

    for (usize bi{0zu}; bi < bodies.size(); ++bi)
    {
        const RigidBody& b = bodies[bi];
        if (b.is_static())
        {
            continue;
        }

        const auto corners = box_world_corners(b);
        for (const Position3& p : corners)
        {
            if (p.z < 0.0f)
            {
                out.push_back(
                    Contact{
                        .body_idx = bi,
                        .p = p,
                        .n = n,
                        .penetration = -p.z,
                    }
                );
            }
        }
    }
}

void apply_impulse_ground(RigidBody& a, const Contact& c, f32 restitution) noexcept
{
    if (a.is_static())
    {
        return;
    }
    // "The quantity eps is called the coefficient of restitution (...)"
    const f32 eps = restitution;
    // (...) and must satisfy 0 <= eps <= 1. If eps = 1 then v_rel^+ = -v_rel^-,
    // and the collision is perfectly 'bouncy'; in particular, no kinetic energy is lost.
    // At the other end of the spectrum, eps = 0 results in v_rel^+ = 0, and a maxium
    // of kinetic energy is lost.
    assert(((eps >= 0.0f) && (eps <= 1.0f)) && "Coefficient of restitution must be in [0, 1]");

    const Direction3 n_hat{c.n};  // Unit Surface Normal

    const Position3 p_a{c.p};                      // Contact point
    const Position3 x_a{a.position};               // Center of mass
    const Direction3 v_a{a.velocity};              // Velocity
    const Direction3 r_a{p_a - x_a};               // Displacement
    const Direction3 omega_a{a.angular_velocity};  // Angular Velocity
    const Direction3 rn{glm::cross(r_a, n_hat)};   // Angular impulse direction

    // [8-1] p_a_dot = v_a + omega_a x (p_a - x_a)
    const Direction3 p_a_dot{v_a + glm::cross(omega_a, r_a)};
    // [8-2] p_b_dot = v_b + omega_b x (p_b - x_b)
    // Can set to 0 b.c. collider body (ground) is static
    const Direction3 p_b_dot{0.0f, 0.0f, 0.0f};
    // [8-3] v_rel = n^ . (p_a_dot) // Normal Relative Velocity
    const f32 v_rel = glm::dot(n_hat, p_a_dot - p_b_dot);

    if (v_rel >= 0.0f)
    {
        // vn == 0 is resting contact, later if resting for some period then we stop the contact
        // force as a optimisation.
        // vn > 0 means we already move away so there's nothing to correct
        return;
    }

    // Will omit the derivation, can express the v_a^+ and v_a^- as well as omega_a^+ and omega_a^-
    // conditions both via coefficient of restitution as well as the impulse identity and solve
    // for the magnitude j (the direction is the unit surface normal).
    // [8-18] Denominator
    const Direction3 invI_rn = a.inv_inertia_world * rn;
    // "Effective Mass" denominator
    const f32 k = a.inv_mass + glm::dot(n_hat, glm::cross(invI_rn, r_a));
    if (k <= 1e-12f)
    {
        return;
    }
    const f32 j = -(1.0f + eps) * v_rel / k;
    // [8-7] Impulse
    const Direction3 J = j * n_hat;

    // [8-5] Delta v = J / M
    a.velocity += J * a.inv_mass;
    // [8-6] tau_impulse = (p - x(t)) x J
    const Direction3 tau_impulse{glm::cross(r_a, J)};
    // "The change in angular velocity is simply I^{-1}(t_0)\tau_{impulse}"
    a.angular_velocity += a.inv_inertia_world * tau_impulse;
}

void positional_correction_ground(
    std::vector<RigidBody>& bodies, const std::vector<Contact>& contacts
)
{
    constexpr f32 slop{0.001f};
    constexpr f32 percent{0.80f};

    std::vector<f32> max_pen;
    max_pen.resize(bodies.size(), 0.0f);

    for (const Contact& c : contacts)
    {
        max_pen[c.body_idx] = std::max(max_pen[c.body_idx], c.penetration);
    }

    for (usize i{0zu}; i < bodies.size(); ++i)
    {
        RigidBody& b = bodies[i];
        if (b.is_static())
        {
            continue;
        }

        const f32 p = max_pen[i];
        if (p <= slop)
        {
            continue;
        }

        b.position.z += percent * (p - slop);
    }
}

}  // namespace

void PhysicsContext::step()
{
    const Duration dt = time_step;
    const f32 dt_s = dt_f32(dt);

    constexpr int solver_iterations{16};

    constexpr f32 restitution{0.1f};
    [[maybe_unused]] constexpr f32 friction{0.0f};

    for (RigidBody& b : bodies)
    {
        if (b.is_static())
        {
            b.inv_inertia_world = glm::mat3(0.0f);
            continue;
        }
        b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
    }

    for (RigidBody& b : bodies)
    {
        if (b.is_static())
        {
            continue;
        }

        const f32 m = 1.0f / b.inv_mass;
        b.force_accum += (m * k_gravity);

        const Direction3 a = b.force_accum * b.inv_mass;
        b.velocity += a * dt_s;

        const Direction3 alpha = b.inv_inertia_world * b.torque_accum;
        b.angular_velocity += alpha * dt_s;
    }

    for (RigidBody& b : bodies)
    {
        if (b.is_static())
        {
            continue;
        }

        b.position += b.velocity * dt_s;
        b.orientation = integrate_orientation(b.orientation, b.angular_velocity, dt_s);

        b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
    }

    std::vector<Contact> contacts{};
    generate_ground_contacts(bodies, contacts);

    for (int it{0}; it < solver_iterations; ++it)
    {
        for (const Contact& c : contacts)
        {
            RigidBody& b = bodies[c.body_idx];
            apply_impulse_ground(b, c, restitution);
        }
    }

    positional_correction_ground(bodies, contacts);

    for (RigidBody& b : bodies)
    {
        b.force_accum = Direction3{0.0f, 0.0f, 0.0f};
        b.torque_accum = Direction3{0.0f, 0.0f, 0.0f};
    }

    time = time + std::chrono::duration_cast<Clock::duration>(dt);
}

}  // namespace ds_pba
