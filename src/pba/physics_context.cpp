// pba/physics_context.cpp
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/physics_context.hpp"
//
#include "pba/core_types.hpp"
#include "pba/format.hpp"  // IWYU pragma: keep

namespace ds_pba
{
namespace
{

static void reduce_contact_points_4(std::vector<Position3>& pts, const Direction3& n) noexcept
{
    if (pts.size() <= 4)
    {
        return;
    }

    // Build tangent basis from normal
    Direction3 t1 = glm::cross(n, Direction3{1.0f, 0.0f, 0.0f});
    if (glm::dot(t1, t1) < 1e-8f)
    {
        t1 = glm::cross(n, Direction3{0.0f, 1.0f, 0.0f});
    }
    t1 = glm::normalize(t1);
    const Direction3 t2 = glm::normalize(glm::cross(n, t1));

    auto pick_extremes = [&](const Direction3& axis) -> std::pair<usize, usize>
    {
        usize i_min{0zu};
        usize i_max{0zu};
        f32 mn = glm::dot(pts[0], axis);
        f32 mx = mn;

        for (usize i{1zu}; i < pts.size(); ++i)
        {
            const f32 d = glm::dot(pts[i], axis);
            if (d < mn)
            {
                mn = d;
                i_min = i;
            }
            if (d > mx)
            {
                mx = d;
                i_max = i;
            }
        }
        return {i_min, i_max};
    };

    const auto [a0, a1] = pick_extremes(t1);
    const auto [b0, b1] = pick_extremes(t2);

    std::array<usize, 4> idx{a0, a1, b0, b1};

    // Deduplicate indices (can happen if points are few/colinear)
    std::vector<Position3> reduced;
    reduced.reserve(4);
    for (usize k{0zu}; k < idx.size(); ++k)
    {
        const Position3 p = pts[idx[k]];
        bool dup{false};
        for (const Position3& q : reduced)
        {
            const Direction3 d = p - q;
            if (glm::dot(d, d) < 1e-8f)
            {
                dup = true;
                break;
            }
        }
        if (!dup)
        {
            reduced.push_back(p);
        }
        if (reduced.size() == 4)
        {
            break;
        }
    }

    pts = std::move(reduced);
}

[[nodiscard]] f32 dt_f32(const Duration& dt) noexcept
{
    return static_cast<f32>(dt.count());
}

[[nodiscard]] Quaternion
integrate_orientation(const Quaternion& q, const Direction3& omega_world, f32 dt) noexcept
{
    const Quaternion wq{0.0f, omega_world.x, omega_world.y, omega_world.z};
    const Quaternion out = q + (0.5f * dt) * (wq * q);
    return glm::normalize(out);
}

[[nodiscard]] glm::mat3
inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
{
    const glm::mat3 R = glm::mat3_cast(q);
    return R * inv_inertia_body * glm::transpose(R);
}

[[nodiscard]] std::array<Direction3, 3> obb_axes_world(const RigidBody& b) noexcept
{
    const glm::mat3 R = glm::mat3_cast(b.orientation);
    return {
        glm::normalize(R * Direction3{1.0f, 0.0f, 0.0f}),
        glm::normalize(R * Direction3{0.0f, 1.0f, 0.0f}),
        glm::normalize(R * Direction3{0.0f, 0.0f, 1.0f}),
    };
}

[[nodiscard]] std::array<Position3, 8> box_world_corners(const RigidBody& b) noexcept
{
    const auto axes = obb_axes_world(b);
    const Direction3 ax = axes[0];
    const Direction3 ay = axes[1];
    const Direction3 az = axes[2];

    const Direction3 he = b.half_extents;

    const Direction3 ex = ax * he.x;
    const Direction3 ey = ay * he.y;
    const Direction3 ez = az * he.z;

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

[[nodiscard]] bool point_in_obb(const Position3& p, const RigidBody& b) noexcept
{
    const auto axes = obb_axes_world(b);
    const Direction3 d = p - b.position;

    const f32 lx = glm::dot(d, axes[0]);
    const f32 ly = glm::dot(d, axes[1]);
    const f32 lz = glm::dot(d, axes[2]);

    const Direction3 he = b.half_extents;

    return (std::abs(lx) <= he.x + 1e-6f) && (std::abs(ly) <= he.y + 1e-6f)
           && (std::abs(lz) <= he.z + 1e-6f);
}

void project_obb_on_axis(
    const RigidBody& b, const Direction3& axis, f32& out_min, f32& out_max
) noexcept
{
    const auto axes = obb_axes_world(b);
    const Direction3 he = b.half_extents;

    const f32 c = glm::dot(b.position, axis);

    const f32 r = std::abs(glm::dot(axes[0], axis)) * he.x
                  + std::abs(glm::dot(axes[1], axis)) * he.y
                  + std::abs(glm::dot(axes[2], axis)) * he.z;

    out_min = c - r;
    out_max = c + r;
}

[[nodiscard]] bool sat_obb_obb(
    const RigidBody& a, const RigidBody& b, Direction3& out_n, f32& out_penetration
) noexcept
{
    const auto ax = obb_axes_world(a);
    const auto bx = obb_axes_world(b);

    // Candidate separating axes:
    // 3 from A, 3 from B, 9 cross products
    std::array<Direction3, 15> axes{};
    {
        axes[0] = ax[0];
        axes[1] = ax[1];
        axes[2] = ax[2];

        axes[3] = bx[0];
        axes[4] = bx[1];
        axes[5] = bx[2];

        usize k{6zu};
        for (usize i{0zu}; i < 3zu; ++i)
        {
            for (usize j{0zu}; j < 3zu; ++j)
            {
                axes[k++] = glm::cross(ax[i], bx[j]);
            }
        }
    }

    const Direction3 d = a.position - b.position;

    f32 best_overlap = std::numeric_limits<f32>::infinity();
    Direction3 best_axis{0.0f, 0.0f, 1.0f};

    for (const Direction3& raw_axis : axes)
    {
        const f32 len2 = glm::dot(raw_axis, raw_axis);
        if (len2 <= 1e-10f)
        {  // Parallel axes => cross product is ~0; skip.
            continue;
        }

        const Direction3 axis = raw_axis / std::sqrt(len2);

        f32 a_min{}, a_max{};
        f32 b_min{}, b_max{};
        project_obb_on_axis(a, axis, a_min, a_max);
        project_obb_on_axis(b, axis, b_min, b_max);

        const f32 overlap = std::min(a_max, b_max) - std::max(a_min, b_min);
        if (overlap <= 0.0f)
        {
            return false;
        }

        if (overlap < best_overlap)
        {
            best_overlap = overlap;

            // Ensure normal points from b -> a
            const f32 s = glm::dot(d, axis);
            best_axis = (s >= 0.0f) ? axis : -axis;
        }
    }

    out_n = best_axis;
    out_penetration = best_overlap;
    return true;
}

void generate_obb_contacts(const std::vector<RigidBody>& bodies, std::vector<Contact>& out)
{
    out.clear();
    out.reserve(bodies.size() * 4zu);

    for (usize i{0zu}; i < bodies.size(); ++i)
    {
        for (usize j{i + 1zu}; j < bodies.size(); ++j)
        {
            const RigidBody& a = bodies[i];
            const RigidBody& b = bodies[j];

            if (a.is_static() && b.is_static())
            {
                continue;
            }

            Direction3 n{0.0f, 0.0f, 1.0f};
            f32 penetration{0.0f};
            if (!sat_obb_obb(a, b, n, penetration))
            {
                continue;
            }

            // Collect candidate points: corners of A inside B and corners of B inside A.
            std::vector<Position3> pts{};
            pts.reserve(16);

            const auto a_corners = box_world_corners(a);
            for (const Position3& p : a_corners)
            {
                if (point_in_obb(p, b))
                {
                    pts.push_back(p);
                }
            }

            const auto b_corners = box_world_corners(b);
            for (const Position3& p : b_corners)
            {
                if (point_in_obb(p, a))
                {
                    pts.push_back(p);
                }
            }

            if (pts.empty())
            {
                // Fallback: use midpoint between centers projected along normal.
                const Position3 p = 0.5f * (a.position + b.position);
                out.push_back(
                    Contact{.a_idx = i, .b_idx = j, .p = p, .n = n, .penetration = penetration}
                );
                continue;
            }

            reduce_contact_points_4(pts, n);

            for (const Position3& p : pts)
            {
                out.push_back(
                    Contact{.a_idx = i, .b_idx = j, .p = p, .n = n, .penetration = penetration}
                );
            }
        }
    }
}

void apply_impulse_contact(
    RigidBody& a, RigidBody& b, const Contact& c, f32 restitution, f32 dt_s
) noexcept
{
    if (a.is_static() && b.is_static())
    {
        return;
    }
    // "The quantity eps is called the coefficient of restitution (...)"
    constexpr f32 k_restitution_threshold{1.0f};
    f32 eps{restitution};
    // (...) and must satisfy 0 <= eps <= 1. If eps = 1 then v_rel^+ = -v_rel^-,
    // and the collision is perfectly 'bouncy'; in particular, no kinetic energy is lost.
    // At the other end of the spectrum, eps = 0 results in v_rel^+ = 0, and a maxium
    // of kinetic energy is lost.
    assert(((eps >= 0.0f) && (eps <= 1.0f)) && "Coefficient of restitution must be in [0, 1]");

    // n^(t_0) the unit surface normal. Points from b to a
    const Direction3 n_hat{c.n};
    assert(std::abs(glm::length(n_hat) - 1.0f) < 1e-4f && "Contact normal must be unit length");

    // Contact Point
    const Position3 p_a{c.p};
    const Position3 p_b{c.p};
    assert(p_a == p_b);

    // Center of mass
    const Position3 x_a{a.position};
    const Position3 x_b{b.position};

    // (Linear) Velocity
    const Direction3 v_a{a.velocity};
    const Direction3 v_b{b.velocity};

    // Displacement from COM (TODO what is that?)
    const Direction3 r_a{p_a - x_a};
    const Direction3 r_b{p_b - x_b};

    // Angular velocities
    const Direction3 omega_a{a.angular_velocity};
    const Direction3 omega_b{b.angular_velocity};

    // [8-1] p_a'(t_0) = v_a(t_0) + omega_a(t_0) x (p_a(t_0) - x_a(t_0))
    const Direction3 p_a_dot{v_a + glm::cross(omega_a, r_a)};
    // [8-2] p_b'(t_0) = v_b(t_0) + omega_b(t_0) x (p_b(t_0) - x_b(t_0))
    const Direction3 p_b_dot{v_b + glm::cross(omega_b, r_b)};

    // [8-3] v_rel = n^ . (p_a'(t_0) - p_b'(t_0)) // Normal Relative Velocity
    const f32 v_rel = glm::dot(n_hat, p_a_dot - p_b_dot);

    // Impulse is some J = j * n^ where we can derive j based on constraints.
    // Will omit the derivation, can express the v_a^+ and v_a^- as well as omega_a^+ and omega_a^-
    // conditions both via coefficient of restitution as well as the impulse identity and solve
    // for the magnitude j

    // Angular impulse direction
    const Direction3 r_axn{glm::cross(r_a, n_hat)};
    const Direction3 r_bxn{glm::cross(r_b, n_hat)};
    const Direction3 invI_r_axn{a.inv_inertia_world * r_axn};
    const Direction3 invI_r_bxn{b.inv_inertia_world * r_bxn};

    // [8-18] Denominator
    // "Effective Mass" denominator
    const f32 k_a{a.inv_mass + glm::dot(n_hat, glm::cross(invI_r_axn, r_a))};
    const f32 k_b{b.inv_mass + glm::dot(n_hat, glm::cross(invI_r_bxn, r_b))};
    const f32 k{k_a + k_b};
    if (k_a + k_b <= 1e-12f)
    {
        return;
    }

    if (-v_rel < k_restitution_threshold)
    {
        eps = 0.0f;
    }

    constexpr f32 slop = 0.001f;
    constexpr f32 beta = 0.2f;  // 0..1, tune
    const f32 pen = std::max(0.0f, c.penetration - slop);
    const f32 bias = -(beta / dt_s) * pen;
    const f32 j{-((1.0f + eps) * v_rel + bias) / k};

    if (j <= 0.0f)
    {
        return;
    }

    // [8-7] Impulse
    const Direction3 J = j * n_hat;

    // [8-5] Delta v = J / M
    if (!a.is_static())
    {
        a.velocity += J * a.inv_mass;
    }
    if (!b.is_static())
    {
        // Recall n^(t_0) points from b to a so we have to reverse impulse direction
        b.velocity -= J * b.inv_mass;
    }
    // [8-6] tau_impulse = (p - x(t)) x J
    // "The change in angular velocity is simply I^{-1}(t_0)\tau_{impulse}"
    if (!a.is_static())
    {
        const Direction3 tau_a_impulse{glm::cross(r_a, J)};
        a.angular_velocity += a.inv_inertia_world * tau_a_impulse;
    }
    if (!b.is_static())
    {
        // Recall n^(t_0) points from b to a so we have to reverse impulse direction
        const Direction3 tau_b_impulse{glm::cross(r_b, J)};
        b.angular_velocity -= b.inv_inertia_world * tau_b_impulse;
    }
}

void positional_correction_contacts(
    std::vector<RigidBody>& bodies, const std::vector<Contact>& contacts
) noexcept
{
    // This corresponds to [Box2D] linearSlop parameter
    constexpr f32 pen_tolerance{0.001f};
    // Percentage of remaining penetration corrected per step
    constexpr f32 percent{0.25f};

    for (const Contact& c : contacts)
    {
        RigidBody& a = bodies[c.a_idx];
        RigidBody& b = bodies[c.b_idx];

        if (a.is_static() && b.is_static())
        {
            continue;
        }

        const f32 pen = c.penetration;
        if (pen <= pen_tolerance)
        {
            continue;
        }

        const Direction3 n_hat = c.n;

        const f32 inv_mass_a = a.inv_mass;
        const f32 inv_mass_b = b.inv_mass;
        const f32 inv_mass_sum = inv_mass_a + inv_mass_b;

        if (inv_mass_sum <= 1e-12f)
        {
            continue;
        }

        const Direction3 correction = (percent * (pen - pen_tolerance) / inv_mass_sum) * n_hat;

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

void PhysicsContext::step()
{
    const Duration dt = time_step;
    const f32 dt_s = dt_f32(dt);

    constexpr int solver_iterations{16};
    constexpr int position_iterations{8};
    constexpr f32 restitution{0.1f};

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
    generate_obb_contacts(bodies, contacts);

    for (usize i{0zu}; i < solver_iterations; ++i)
    {
        for (const Contact& c : contacts)
        {
            RigidBody& a = bodies[c.a_idx];
            RigidBody& b = bodies[c.b_idx];
            apply_impulse_contact(a, b, c, restitution, dt_s);
        }
    }

    for (usize i{0zu}; i < position_iterations; ++i)
    {
        std::vector<Contact> pos_contacts{};
        generate_obb_contacts(bodies, pos_contacts);
        positional_correction_contacts(bodies, pos_contacts);
    }

    for (RigidBody& b : bodies)
    {
        b.force_accum = Direction3{0.0f, 0.0f, 0.0f};
        b.torque_accum = Direction3{0.0f, 0.0f, 0.0f};
    }

    time = time + std::chrono::duration_cast<Clock::duration>(dt);
}

}  // namespace ds_pba
