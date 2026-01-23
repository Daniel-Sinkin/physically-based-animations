// pba/physics_context.cpp
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/physics_context.hpp"
//

namespace ds_pba
{
namespace
{

[[nodiscard]] f32 dt_f32(const Duration& dt) noexcept
{
    return static_cast<f32>(dt.count());
}

[[maybe_unused]] [[nodiscard]] glm::mat3
inv_inertia_body_box(f32 inv_mass, const Direction3& he) noexcept
{
    if (inv_mass == k_static_mass)
    {
        return glm::mat3(0.0f);
    }

    const f32 m = 1.0f / inv_mass;
    const f32 x = 2.0f * he.x;
    const f32 y = 2.0f * he.y;
    const f32 z = 2.0f * he.z;

    // Box inertia tensor about center of mass:
    // Ixx = (1/12) m (y^2 + z^2), etc.
    const f32 Ixx = (m / 12.0f) * (y * y + z * z);
    const f32 Iyy = (m / 12.0f) * (x * x + z * z);
    const f32 Izz = (m / 12.0f) * (x * x + y * y);

    glm::mat3 invI(0.0f);
    invI[0][0] = (Ixx > 0.0f) ? (1.0f / Ixx) : 0.0f;
    invI[1][1] = (Iyy > 0.0f) ? (1.0f / Iyy) : 0.0f;
    invI[2][2] = (Izz > 0.0f) ? (1.0f / Izz) : 0.0f;
    return invI;
}

[[nodiscard]] glm::mat3
inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
{
    const glm::mat3 R = glm::mat3_cast(q);
    return R * inv_inertia_body * glm::transpose(R);
}

[[nodiscard]] Quaternion
integrate_orientation(const Quaternion& q, const Direction3& omega_world, f32 dt) noexcept
{
    // q_dot = 0.5 * [0, omega] * q  (omega in world space)
    const Quaternion wq{0.0f, omega_world.x, omega_world.y, omega_world.z};
    Quaternion out = q + (0.5f * dt) * (wq * q);
    return glm::normalize(out);
}

}  // namespace

void PhysicsContext::step()
{
    const Duration dt = time_step;
    const f32 dt_s = dt_f32(dt);

    // 1) Apply global forces (gravity) into accumulators
    for (RigidBody& b : bodies)
    {
        if (b.is_static())
        {
            continue;
        }

        // F = m g  -> a = g; we can apply as force (consistent with accumulators)
        const f32 m = 1.0f / b.inv_mass;
        b.force_accum += (m * k_gravity);

        // Ensure world inertia is up-to-date even if only forces act this frame
        b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
    }

    // 2) Integrate velocities (semi-implicit Euler)
    for (RigidBody& b : bodies)
    {
        if (b.is_static())
        {
            continue;
        }

        // Linear
        const Direction3 a = b.force_accum * b.inv_mass;
        b.velocity += a * dt_s;

        // Angular (basic; ignores gyroscopic term for now)
        const Direction3 alpha = b.inv_inertia_world * b.torque_accum;
        b.angular_velocity += alpha * dt_s;
    }

    // 3) Integrate pose
    for (RigidBody& b : bodies)
    {
        if (b.is_static())
        {
            continue;
        }

        b.position += b.velocity * dt_s;
        b.orientation = integrate_orientation(b.orientation, b.angular_velocity, dt_s);

        // Keep derived inertia in sync with the new orientation
        b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);

        // Clear accumulators
        b.force_accum = Direction3{0.0f, 0.0f, 0.0f};
        b.torque_accum = Direction3{0.0f, 0.0f, 0.0f};
    }

    time = time + std::chrono::duration_cast<Clock::duration>(dt);
}

}  // namespace ds_pba
