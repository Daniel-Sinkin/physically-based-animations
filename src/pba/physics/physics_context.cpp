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

[[nodiscard]] f32
compute_total_kinetic_energy(std::span<const RigidBody> bodies, bool include_angular) noexcept
{
    f32 E{0.0f};
    for (const RigidBody& b : bodies)
    {
        if (b.inv_mass <= 0.0f)
        {
            continue;
        }
        const f32 m = 1.0f / b.inv_mass;
        E += 0.5f * m * glm::dot(b.velocity, b.velocity);

        if (include_angular)
        {
            // TODO: Cache this directly instead of re-inverting every time
            const glm::mat3 I_world = glm::inverse(b.inv_inertia_world);
            E += 0.5f * glm::dot(b.angular_velocity, I_world * b.angular_velocity);
        }
    }
    return E;
}
}  // namespace

void PhysicsContext::step()
{
    step_arena.release();
    Contacts contacts{&step_arena};

    const Duration dt{time_step};
    const f32 dt_s{dt_f32(dt)};

    // Update inertia based on  sg
    update_inv_inertia_world(bodies);

    for (const auto& f : external_forces)
    {
        f.fn(bodies, dt_s, f.user);
    }

    integrate_forces(bodies, dt_s);
    integrate_velocities(bodies, dt_s);

    generate_obb_contacts(bodies, contacts);
    if constexpr (k_validate_contacts)
    {
        for ([[maybe_unused]] const auto& c : contacts)
        {
            assert(c.is_valid());
        }
    }

    {  // Setup debug info for this step
        debug_contacts.clear();
        debug_contacts.reserve(contacts.size());
        for (const Contact& c : contacts)
        {
            const RigidBody& a = bodies[c.a_idx];
            const RigidBody& b = bodies[c.b_idx];
            debug_contacts.push_back(
                DebugContact{
                    .a_id = a.id,
                    .b_id = b.id,
                    .p = c.p,
                    .n = c.n,
                    .penetration = c.penetration,
                    .allow_warm_start = c.allow_warm_start,
                }
            );
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

    for (Contact& contact : contacts)
    {
        if (!contact.allow_warm_start)
        {
            continue;
        }
        if (contact.penetration < 0.05f)
        {
            warm_start_contact(bodies, contact);
        }
    }

    solve_velocity_constraints(bodies, contacts, dt_s);
    solve_position_constraints(bodies, contacts);

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

    apply_sleep_and_damping(bodies, dt_s);
    clear_accumulators(bodies);

    debug_total_kinetic_energy = compute_total_kinetic_energy(bodies, true);

    debug_energy_sample_accum += dt;
    while (debug_energy_sample_accum >= Duration{k_energy_sample_dt})
    {
        debug_energy_sample_accum -= Duration{k_energy_sample_dt};
        debug_total_kinetic_energy_history.push_back(debug_total_kinetic_energy);

        if (debug_total_kinetic_energy_history.size() > k_energy_history_len)
        {
            const usize overflow{debug_total_kinetic_energy_history.size() - k_energy_history_len};
            using Diff = std::vector<f32>::difference_type;
            debug_total_kinetic_energy_history.erase(
                debug_total_kinetic_energy_history.begin(),
                debug_total_kinetic_energy_history.begin() + static_cast<Diff>(overflow)
            );
        }
    }
    time = time + std::chrono::duration_cast<Clock::duration>(dt);
}

}  // namespace ds_pba
