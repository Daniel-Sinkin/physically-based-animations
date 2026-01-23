#include "pba/physics_context.hpp"

#include <algorithm>
#include <chrono>

namespace ds_pba
{

f32 center_axis(f32 mn, f32 mx) noexcept
{
    return 0.5f * (mn + mx);
}

Contact aabb_contact(const AABB& a, const AABB& b) noexcept
{
    const f32 ox = std::min(a.max.x, b.max.x) - std::max(a.min.x, b.min.x);
    const f32 oy = std::min(a.max.y, b.max.y) - std::max(a.min.y, b.min.y);
    const f32 oz = std::min(a.max.z, b.max.z) - std::max(a.min.z, b.min.z);

    if (ox <= 0.0f || oy <= 0.0f || oz <= 0.0f)
    {
        return {};
    }

    Contact c{};
    c.hit = true;

    const f32 acx = center_axis(a.min.x, a.max.x);
    const f32 bcx = center_axis(b.min.x, b.max.x);
    const f32 acy = center_axis(a.min.y, a.max.y);
    const f32 bcy = center_axis(b.min.y, b.max.y);
    const f32 acz = center_axis(a.min.z, a.max.z);
    const f32 bcz = center_axis(b.min.z, b.max.z);

    c.penetration = ox;
    c.normal = {(acx < bcx) ? 1.0f : -1.0f, 0.0f, 0.0f};

    if (oy < c.penetration)
    {
        c.penetration = oy;
        c.normal = {0.0f, (acy < bcy) ? 1.0f : -1.0f, 0.0f};
    }
    if (oz < c.penetration)
    {
        c.penetration = oz;
        c.normal = {0.0f, 0.0f, (acz < bcz) ? 1.0f : -1.0f};
    }

    return c;
}

AABB RigidBody::get_world_collider() const
{
    return {.min = collider.min + position, .max = collider.max + position};
}

bool RigidBody::is_static() const noexcept
{
    return inv_mass == k_static_mass;
}

void resolve_contact(RigidBody& a, RigidBody& b, const Contact& c, f32 restitution)
{
    const f32 inv_ma = a.inv_mass;
    const f32 inv_mb = b.inv_mass;
    const f32 inv_m_sum = inv_ma + inv_mb;

    if (inv_m_sum <= 0.0f)
    {
        return;
    }

    constexpr f32 slop = 0.001f;
    constexpr f32 correction_coeff = 0.8f;

    const f32 corr_mag = correction_coeff * std::max(c.penetration - slop, 0.0f) / inv_m_sum;

    const Direction3 correction = corr_mag * c.normal;

    if (inv_ma > 0.0f)
    {
        a.position -= correction * inv_ma;
    }
    if (inv_mb > 0.0f)
    {
        b.position += correction * inv_mb;
    }

    const Direction3 rv = b.velocity - a.velocity;
    const f32 normal_direction = glm::dot(rv, c.normal);

    if (normal_direction > 0.0f)
    {
        return;
    }

    const f32 j = -(1.0f + restitution) * normal_direction / inv_m_sum;
    const Direction3 impulse = j * c.normal;

    if (inv_ma > 0.0f)
    {
        a.velocity -= impulse * inv_ma;
    }
    if (inv_mb > 0.0f)
    {
        b.velocity += impulse * inv_mb;
    }
}

void PhysicsContext::apply_forces()
{
    for (RigidBody& body : bodies)
    {
        if (!body.is_static())
        {
            body.velocity += k_gravity * static_cast<f32>(time_step.count());
        }
    }
}

void PhysicsContext::update_positions()
{
    for (RigidBody& body : bodies)
    {
        if (body.is_static())
        {
            continue;
        }

        body.position += body.velocity * static_cast<f32>(time_step.count());

        const f32 r2 = glm::dot(body.position, body.position);
        if (r2 > 200.0f * 200.0f)
        {
            body.inv_mass = k_static_mass;
        }
    }
}

void PhysicsContext::solve_collisions()
{
    constexpr f32 restitution = 0.1f;

    for (usize i = 0; i < bodies.size(); ++i)
    {
        for (usize j = i + 1; j < bodies.size(); ++j)
        {
            RigidBody& a = bodies[i];
            RigidBody& b = bodies[j];

            const Contact c = aabb_contact(a.get_world_collider(), b.get_world_collider());

            if (c.hit)
            {
                resolve_contact(a, b, c, restitution);
            }
        }
    }
}

void PhysicsContext::step()
{
    apply_forces();
    update_positions();
    solve_collisions();

    time = time + std::chrono::duration_cast<Clock::duration>(time_step);
}

}  // namespace ds_pba
