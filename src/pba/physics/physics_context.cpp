// pba/physics/physics_context.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/physics/physics_context.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/physics/collision.hpp"
#include "pba/physics/solver.hpp"
//
#include <chrono>
#include <vector>

namespace ds_pba
{
namespace
{

[[nodiscard]] f32 dt_f32(const Duration& dt) noexcept
{
    return static_cast<f32>(dt.count());
}

}  // namespace

void PhysicsContext::step()
{
    const Duration dt{time_step};
    const f32 dt_s{dt_f32(dt)};

    physics_update_inv_inertia_world(bodies);

    physics_integrate_forces(bodies, dt_s);
    physics_integrate_velocities(bodies, dt_s);

    std::vector<Contact> contacts{};
    generate_obb_contacts(bodies, contacts);
    if constexpr (k_validate_contacts)
    {
        for ([[maybe_unused]] const auto& c : contacts)
        {
            assert(c.is_valid());
        }
    }

    // Pull warm-start state from cache into contacts before warm start + solve.
    for (Contact& c : contacts)
    {
        if (!c.allow_warm_start)
        {
            continue;
        }
        const RigidBody& a = bodies[c.a_idx];
        const RigidBody& b = bodies[c.b_idx];

        const ContactKey key = make_contact_key(a, b, c.p);
        if (auto it = contact_cache.find(key); it != contact_cache.end())
        {
            c.lambda_n = it->second.lambda_n;
            c.lambda_t = it->second.lambda_t;
            c.t_hat = it->second.t_hat;
            c.has_t_hat = it->second.has_t_hat;
        }
    }

    physics_warm_start_contacts(bodies, contacts);

    physics_solve_velocity_constraints(bodies, contacts, dt_s);

    physics_solve_position_constraints(bodies, contacts);

    contact_cache.clear();
    contact_cache.reserve(contacts.size() * 2zu);

    for (const Contact& c : contacts)
    {
        if (!c.allow_warm_start)
        {
            continue;
        }
        const RigidBody& a = bodies[c.a_idx];
        const RigidBody& b = bodies[c.b_idx];

        const ContactKey key = make_contact_key(a, b, c.p);

        contact_cache.insert_or_assign(
            key,
            ContactCacheEntry{
                .lambda_n = c.lambda_n,
                .lambda_t = c.lambda_t,
                .t_hat = c.t_hat,
                .has_t_hat = c.has_t_hat,
            }
        );
    }

    physics_apply_sleep_and_damping(bodies, dt_s);
    physics_clear_accumulators(bodies);

    time = time + std::chrono::duration_cast<Clock::duration>(dt);
}

}  // namespace ds_pba
