// pba/physics_context.cpp
#include "pba/math_types.hpp"
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/physics_context.hpp"
//
#include "pba/constants.hpp"
#include "pba/core_types.hpp"
#include "pba/format.hpp"  // IWYU pragma: keep

#include <glm/geometric.hpp>

namespace ds_pba
{
namespace
{
static void reduce_contact_points_4(
    std::array<Position3, k_contact_points>& pts, usize& pt_count, const Direction3& n
) noexcept
{
    if (pt_count <= 4)
    {
        return;
    }

    Direction3 t1{glm::cross(n, Direction3{1.0f, 0.0f, 0.0f})};
    if (glm::dot(t1, t1) < 1e-8f)
    {
        t1 = glm::cross(n, Direction3{0.0f, 1.0f, 0.0f});
    }
    t1 = glm::normalize(t1);
    const Direction3 t2{glm::normalize(glm::cross(n, t1))};

    auto pick_extremes = [&](const Direction3& axis) -> std::pair<usize, usize>
    {
        usize i_min{0zu};
        usize i_max{0zu};
        f32 mn = glm::dot(pts[0], axis);
        f32 mx = mn;

        for (usize i{1zu}; i < pt_count; ++i)
        {
            const f32 d{glm::dot(pts[i], axis)};
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

    std::array<Position3, k_collision_reduced_num> reduced{};
    usize reduced_count{0zu};

    for (usize k{0zu}; k < idx.size(); ++k)
    {
        const Position3 p{pts[idx[k]]};

        bool dup{false};
        for (usize r{0zu}; r < reduced_count; ++r)
        {
            const Direction3 d{p - reduced[r]};
            if (glm::dot(d, d) < 1e-8f)
            {
                dup = true;
                break;
            }
        }

        if (!dup)
        {
            reduced[reduced_count++] = p;
            if (reduced_count == 4)
            {
                break;
            }
        }
    }

    // write back
    for (usize i{0zu}; i < reduced_count; ++i)
    {
        pts[i] = reduced[i];
    }
    pt_count = reduced_count;
}

[[nodiscard]] f32 dt_f32(const Duration& dt) noexcept
{
    return static_cast<f32>(dt.count());
}

[[nodiscard]] Quaternion
integrate_orientation(const Quaternion& q, const Direction3& omega_world, f32 dt) noexcept
{
    const Quaternion wq{0.0f, omega_world.x, omega_world.y, omega_world.z};
    const Quaternion out{q + (0.5f * dt) * (wq * q)};
    return glm::normalize(out);
}

[[nodiscard]] glm::mat3
inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
{
    const glm::mat3 R{glm::mat3_cast(q)};
    return R * inv_inertia_body * glm::transpose(R);
}

[[nodiscard]] std::array<Direction3, 3> obb_axes_world(const RigidBody& b) noexcept
{
    const glm::mat3 R{glm::mat3_cast(b.orientation)};

    return {
        glm::normalize(R * k_axis_x),
        glm::normalize(R * k_axis_y),
        glm::normalize(R * k_axis_z),
    };
}

[[nodiscard]] std::array<Position3, 8> box_world_corners(const RigidBody& b) noexcept
{
    const auto [ax, ay, az] = obb_axes_world(b);
    const Direction3 ex{ax * b.half_extents.x};
    const Direction3 ey{ay * b.half_extents.y};
    const Direction3 ez{az * b.half_extents.z};
    return std::array<Position3, 8>{
        b.position - ex - ey - ez,
        b.position - ex - ey + ez,
        b.position - ex + ey - ez,
        b.position - ex + ey + ez,
        b.position + ex - ey - ez,
        b.position + ex - ey + ez,
        b.position + ex + ey - ez,
        b.position + ex + ey + ez,
    };
}

[[nodiscard]] bool point_in_obb(const Position3& p, const RigidBody& b) noexcept
{
    const std::array<Direction3, 3> axes = obb_axes_world(b);
    const Direction3 d{p - b.position};

    const f32 lx{glm::dot(d, axes[0])};
    const f32 ly{glm::dot(d, axes[1])};
    const f32 lz{glm::dot(d, axes[2])};

    const Direction3 he{b.half_extents};
    constexpr f32 eps{1e-6f};

    const bool inside_x{std::abs(lx) <= he.x + eps};
    const bool inside_y{std::abs(ly) <= he.y + eps};
    const bool inside_z{std::abs(lz) <= he.z + eps};

    return inside_x && inside_y && inside_z;
}

void project_obb_on_axis(
    const RigidBody& b, const Direction3& axis, f32& out_min, f32& out_max
) noexcept
{
    const auto axes = obb_axes_world(b);

    const f32 center_proj{glm::dot(b.position, axis)};

    const f32 radius_proj{
        std::abs(glm::dot(axes[0], axis)) * b.half_extents.x
        + std::abs(glm::dot(axes[1], axis)) * b.half_extents.y
        + std::abs(glm::dot(axes[2], axis)) * b.half_extents.z
    };

    out_min = center_proj - radius_proj;
    out_max = center_proj + radius_proj;
}

[[nodiscard]] bool sat_obb_obb(
    const RigidBody& a, const RigidBody& b, Direction3& out_n, f32& out_penetration
) noexcept
{
    const auto ax = obb_axes_world(a);
    const auto bx = obb_axes_world(b);

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

        const f32 overlap{std::min(a_max, b_max) - std::max(a_min, b_min)};
        if (overlap <= 0.0f)
        {
            return false;
        }

        if (overlap < best_overlap)
        {
            best_overlap = overlap;

            // Ensure normal points from b -> a
            const f32 s{glm::dot(d, axis)};
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
    out.reserve(bodies.size() * bodies.size());

    for (usize i{0zu}; i < bodies.size(); ++i)
    {
        for (usize j{i + 1zu}; j < bodies.size(); ++j)
        {
            const RigidBody& a{bodies[i]};
            const RigidBody& b{bodies[j]};

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

            std::array<Position3, k_contact_points> pts{};
            usize pt_count{0};

            const std::array<Position3, 8> a_corners{box_world_corners(a)};
            for (const Position3& p : a_corners)
            {
                if (point_in_obb(p, b))
                {
                    if (pt_count < pts.size())
                    {
                        pts[pt_count++] = p;
                    }
                }
            }

            const std::array<Position3, 8> b_corners{box_world_corners(b)};
            for (const Position3& p : b_corners)
            {
                if (point_in_obb(p, a))
                {
                    if (pt_count < pts.size())
                    {
                        pts[pt_count++] = p;
                    }
                }
            }

            if (pt_count == 0)
            {
                const Position3 p{0.5f * (a.position + b.position)};
                out.push_back(
                    Contact{.a_idx = i, .b_idx = j, .p = p, .n = n, .penetration = penetration}
                );
                continue;
            }

            reduce_contact_points_4(pts, pt_count, n);

            for (usize k{0zu}; k < pt_count; ++k)
            {
                out.push_back(
                    Contact{.a_idx = i, .b_idx = j, .p = pts[k], .n = n, .penetration = penetration}
                );
            }
        }
    }
}

void apply_impulse_contact_friction(
    RigidBody& a,
    RigidBody& b,
    Direction3 r_a,
    Direction3 r_b,
    Direction3 n_hat,
    f32 impulse_magnitude
) noexcept
{
    // This section adapted from [Catto05] (4.3) Friction Constraints.
    // The paper explicitly uses a orthonormal tangent basis {u1, u2} with
    // u1 x u2 = n and computes (v_rel.u1) and (v_rel.u2) seperately.
    // To avoid computing these U:={u1, u2} explicitly this code does
    //
    // proj_U(v_rel) = v_rel - (v_rel.n)*n == (v_rel.u1)*u1 + (v_rel.u2)*u2
    //
    // which is an orthonormal projection onto the orthogonal space of the
    // normal direction (i.e. exactly the tangent space).
    const Direction3 p_a_dot{a.velocity + glm::cross(a.angular_velocity, r_a)};
    const Direction3 p_b_dot{b.velocity + glm::cross(b.angular_velocity, r_b)};
    const Direction3 v_rel_w{p_a_dot - p_b_dot};

    // Projection onto the tangent
    // v_rel.n
    const f32 vrel_dot_n{glm::dot(v_rel_w, n_hat)};
    // v_rel - (v_rel.n) * n
    const Direction3 v_t{v_rel_w - vrel_dot_n * n_hat};
    // Equation (21) would be C_{u1}' = v_rel_w . u1
    // Equation (22) would be C_{u2}' = v_rel_w . u2

    const f32 vt2{glm::dot(v_t, v_t)};
    if (vt2 <= 1e-12f)
    {
        return;
    }
    const f32 vt_len{std::sqrt(vt2)};
    // Model friction as a force acting in the opposite direction of "slip"
    const Direction3 t_hat{v_t / vt_len};

    // These are the angular Jacobian terms (r x u) from Equation (23)
    const Direction3 r_a_cross_t{glm::cross(r_a, t_hat)};
    const Direction3 r_b_cross_t{glm::cross(r_b, t_hat)};

    // Change in angular velocity is given by
    //
    // Delta omega = I^-1(r x J)
    //
    const Direction3 invI_r_axt{a.inv_inertia_world * r_a_cross_t};
    const Direction3 invI_r_bxt{b.inv_inertia_world * r_b_cross_t};

    // Computes effective mass along t_hat, often denoted by just k
    const f32 k2a{a.inv_mass + glm::dot(t_hat, glm::cross(invI_r_axt, r_a))};
    const f32 k2b{b.inv_mass + glm::dot(t_hat, glm::cross(invI_r_bxt, r_b))};
    const f32 effective_mass{k2a + k2b};
    if (effective_mass <= 1e-12f)
    {
        return;
    }

    // Coulomb limit on impulse magnitude
    f32 friction_impulse_magnitude{-glm::dot(v_rel_w, t_hat) / effective_mass};
    const f32 max_jt{k_friction * impulse_magnitude};
    friction_impulse_magnitude = std::clamp(friction_impulse_magnitude, -max_jt, +max_jt);

    const Direction3 friction_impulse{friction_impulse_magnitude * t_hat};

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
    RigidBody& a, RigidBody& b, const Contact& c, f32 restitution, f32 dt_s
) noexcept
{
    // This is based on [Baraff97]
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

    // Displacement from center of mass
    const Direction3 r_a{p_a - x_a};
    const Direction3 r_b{p_b - x_b};

    // Angular velocities are denoted by omega_a and omega_b respectively
    // (8-1) p_a'(t_0) = v_a(t_0) + omega_a(t_0) x (p_a(t_0) - x_a(t_0))
    const Direction3 p_a_dot{v_a + glm::cross(a.angular_velocity, r_a)};
    // (8-2) p_b'(t_0) = v_b(t_0) + omega_b(t_0) x (p_b(t_0) - x_b(t_0))
    const Direction3 p_b_dot{v_b + glm::cross(b.angular_velocity, r_b)};

    // (8-3) v_rel = n^ . (p_a'(t_0) - p_b'(t_0)) // Normal Relative Velocity
    const f32 v_rel = glm::dot(n_hat, p_a_dot - p_b_dot);

    // Impulse is given by J = j * n^ where we can derive j based on constraints.
    // Will omit the derivation, can express the v_a^+ and v_a^- as well as omega_a^+ and omega_a^-
    // conditions both via coefficient of restitution as well as the impulse identity and solve
    // for the magnitude j

    // Angular impulse direction
    const Direction3 r_a_cross_n{glm::cross(r_a, n_hat)};
    const Direction3 r_b_cross_n{glm::cross(r_b, n_hat)};
    const Direction3 invI_r_axn{a.inv_inertia_world * r_a_cross_n};
    const Direction3 invI_r_bxn{b.inv_inertia_world * r_b_cross_n};

    // (8-18) Denominator
    // "Effective Mass" denominator; in the literature often denoted by k
    const f32 k_a{a.inv_mass + glm::dot(n_hat, glm::cross(invI_r_axn, r_a))};
    const f32 k_b{b.inv_mass + glm::dot(n_hat, glm::cross(invI_r_bxn, r_b))};
    const f32 effective_mass{k_a + k_b};
    if (effective_mass <= 1e-12f)
    {
        return;
    }
    if (-v_rel < k_restitution_threshold)
    {
        eps = 0.0f;
    }
    // Baumgarte-style bias
    const f32 effective_pen{std::max(0.0f, c.penetration - k_pen_tolerance)};
    const f32 bias{-(k_pen_correction_frag / dt_s) * effective_pen};
    // In the paper this is denoted by j
    const f32 impulse_magnitude{-((1.0f + eps) * v_rel + bias) / effective_mass};
    if (impulse_magnitude <= 0.0f)
    {
        return;
    }

    // Equation (8-7) Impulse is denoted by J
    const Direction3 impulse{impulse_magnitude * n_hat};

    // Equation (8-5) Delta v = J / M
    if (!a.is_static())
    {
        a.velocity += impulse * a.inv_mass;
    }
    if (!b.is_static())
    {
        // Recall n^(t_0) points from b to a so we have to reverse impulse direction
        b.velocity -= impulse * b.inv_mass;
    }
    // Equation (8-6) tau_impulse = (p - x(t)) x J
    // "The change in angular velocity is simply I^{-1}(t_0)\tau_{impulse}"
    if (!a.is_static())
    {
        const Direction3 tau_a_impulse{glm::cross(r_a, impulse)};
        a.angular_velocity += a.inv_inertia_world * tau_a_impulse;
    }
    if (!b.is_static())
    {
        // Recall n^(t_0) points from b to a so we have to reverse impulse direction
        const Direction3 tau_b_impulse{glm::cross(r_b, impulse)};
        b.angular_velocity -= b.inv_inertia_world * tau_b_impulse;
    }
    apply_impulse_contact_friction(a, b, r_a, r_b, n_hat, impulse_magnitude);
}

void positional_correction_contacts(
    std::vector<RigidBody>& bodies, const std::vector<Contact>& contacts
) noexcept
{

    for (const Contact& c : contacts)
    {
        RigidBody& a = bodies[c.a_idx];
        RigidBody& b = bodies[c.b_idx];

        if (a.is_static() && b.is_static())
        {
            continue;
        }
        const f32 pen{c.penetration};
        if (pen <= k_pen_tolerance)
        {
            continue;
        }

        const Direction3 n_hat{c.n};

        const f32 inv_mass_a{a.inv_mass};
        const f32 inv_mass_b{b.inv_mass};
        const f32 inv_mass_sum{inv_mass_a + inv_mass_b};

        if (inv_mass_sum <= 1e-12f)
        {
            continue;
        }

        const Direction3 correction{
            (k_pen_correction_frag * (pen - k_pen_tolerance) / inv_mass_sum) * n_hat
        };

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
    const Duration dt{time_step};
    const f32 dt_s{dt_f32(dt)};

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

        const f32 m{1.0f / b.inv_mass};
        b.force_accum += (m * k_gravity);

        const Direction3 a{b.force_accum * b.inv_mass};
        b.velocity += a * dt_s;

        const Direction3 alpha{b.inv_inertia_world * b.torque_accum};
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
    if constexpr (k_validate_contacts)
    {
        for (const auto& c : contacts)
        {
            assert(c.is_valid());
        }
    }

    for (usize i{0zu}; i < k_solver_iterations; ++i)
    {
        for (const Contact& c : contacts)
        {
            RigidBody& a{bodies[c.a_idx]};
            RigidBody& b{bodies[c.b_idx]};
            apply_impulse_contact(a, b, c, k_restitution, dt_s);
        }
    }

    for (usize i{0zu}; i < k_position_iterations; ++i)
    {
        positional_correction_contacts(bodies, contacts);
    }

    for (RigidBody& b : bodies)
    {
        if (b.is_static())
        {
            continue;
        }

        const f32 v_sleep{k_linear_sleep_speed_threshold};
        const f32 w_sleep{k_angular_sleep_speed_threshold};

        const f32 v2{glm::dot(b.velocity, b.velocity)};
        const f32 w2{glm::dot(b.angular_velocity, b.angular_velocity)};

        if (v2 < v_sleep * v_sleep)
        {
            b.velocity *= std::exp(-k_linear_damping * dt_s);
        }
        if (w2 < w_sleep * w_sleep)
        {
            b.angular_velocity *= std::exp(-k_angular_damping * dt_s);
        }
    }

    for (RigidBody& b : bodies)
    {
        b.force_accum = Direction3{};
        b.torque_accum = Direction3{};
    }

    time = time + std::chrono::duration_cast<Clock::duration>(dt);
}

}  // namespace ds_pba
