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
#include "pba/physics/forces.hpp"
#include "pba/physics/solver.hpp"
//
#include <chrono>
#include <print>
#include <type_traits>
#include <variant>
#include <vector>

namespace ds_pba
{
namespace
{

[[nodiscard]] auto dt_f32(const Duration& dt) noexcept -> f32
{
    return static_cast<f32>(dt.count());
}

[[nodiscard]] auto
compute_total_kinetic_energy(std::span<const RigidBody> bodies, bool include_angular) noexcept
    -> f32
{
    auto E = 0.0f;
    for (const auto& b : bodies)
    {
        if (b.inv_mass <= 0.0f)
        {
            continue;
        }
        E += 0.5f * glm::dot(b.velocity, b.velocity) / b.inv_mass;
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

auto PhysicsContext::step() -> void
{
    contact_arena.clear();
    // How long the frame lasted, we do physics processing
    // until physics time reaches that time.
    const Duration dt{time_step};
    const f32 dt_s{dt_f32(dt)};

    update_inv_inertia_world(bodies);

    for (auto& b : bodies)
    {
        for (auto& force : simple_forces)
        {
            apply_force(b, force);
        }
    }
    for (auto& force : complex_forces)
    {
        apply_force(bodies, force);
    }

    integrate_forces(bodies, dt_s);
    integrate_velocities(bodies, dt_s);
    debug_collision_stats = generate_obb_contacts(bodies, contact_arena, collision_scratch);

    {  // Setup debug info for this step
        debug_contacts.clear();
        debug_contacts.reserve(contact_arena.used() / sizeof(Contact));
        for (const auto& contact : contact_arena.as_span<Contact>())
        {
            const RigidBody& a = bodies[contact.a_idx];
            const RigidBody& b = bodies[contact.b_idx];
            debug_contacts.push_back(
                DebugContact{
                    .a_id = a.id,
                    .b_id = b.id,
                    .p = contact.p,
                    .n = contact.n,
                    .penetration = contact.penetration,
                    .allow_warm_start = contact.allow_warm_start,
                }
            );
        }
    }

    // Pull warm-start state from cache into contacts before warm start + solve.
    for (auto& contact : contact_arena.as_span<Contact>())
    {
        if (!contact.allow_warm_start)
        {
            continue;
        }
        const RigidBody& a = bodies[contact.a_idx];
        const RigidBody& b = bodies[contact.b_idx];

        const ContactKey key = make_contact_key(a, b, contact.p);
        if (auto it = contact_cache.find(key); it != contact_cache.end())
        {
            contact.lambda_n = it->second.lambda_n;
            contact.lambda_t = it->second.lambda_t;
            contact.t_hat = it->second.t_hat;
            contact.has_t_hat = it->second.has_t_hat;
        }
    }

    for (auto& contact : contact_arena.as_span<Contact>())
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

    solve_velocity_constraints(bodies, contact_arena, dt_s);
    solve_position_constraints(bodies, contact_arena);

    contact_cache.clear();
    contact_cache.reserve((contact_arena.used() / sizeof(Contact)) * 2zu);

    for (const auto& contact : contact_arena.as_span<Contact>())
    {
        if (!contact.allow_warm_start)
        {
            continue;
        }
        const RigidBody& a = bodies[contact.a_idx];
        const RigidBody& b = bodies[contact.b_idx];

        const ContactKey key = make_contact_key(a, b, contact.p);

        contact_cache.insert_or_assign(
            key,
            ContactCacheEntry{
                .lambda_n = contact.lambda_n,
                .lambda_t = contact.lambda_t,
                .t_hat = contact.t_hat,
                .has_t_hat = contact.has_t_hat,
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
        debug_total_kinetic_energy_history.push(debug_total_kinetic_energy);
    }
    time = time + std::chrono::duration_cast<Clock::duration>(dt);
}

auto PhysicsContext::clear() -> void
{
    bodies.clear();
    simple_forces.clear();
    complex_forces.clear();
    contact_cache.clear();
    debug_contacts.clear();
    debug_collision_stats = {};
    debug_total_kinetic_energy = 0.0f;
    debug_energy_sample_accum = Duration{0.0};
    debug_total_kinetic_energy_history.clear();
}

}  // namespace ds_pba
