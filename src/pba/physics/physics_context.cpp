// pba/physics/physics_context.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/physics/physics_context.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/core/parallel_for.hpp"
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
compute_total_kinetic_energy(const RigidBodySOA& bodies, bool include_angular) noexcept
    -> f32
{
    auto E = 0.0f;
    for (auto i = 0zu; i < bodies.size(); ++i)
    {
        if (bodies.inv_masses[i] <= 0.0f)
        {
            continue;
        }
        E += 0.5f * glm::dot(bodies.velocities[i], bodies.velocities[i]) / bodies.inv_masses[i];
        if (include_angular)
        {
            E += 0.5f * glm::dot(
                bodies.angular_velocities[i], bodies.inertia_worlds[i] * bodies.angular_velocities[i]
            );
        }
    }
    return E;
}

auto clear_debug_tracking(PhysicsContext& ctx) noexcept -> void
{
    ctx.debug_contacts.clear();
    ctx.debug_collision_stats = {};
    ctx.debug_total_kinetic_energy = 0.0f;
    ctx.debug_energy_sample_accum = Duration{0.0};
    ctx.debug_total_kinetic_energy_history.clear();
}
}  // namespace

auto PhysicsContext::try_body(BodyHandle h) noexcept -> std::optional<BodyRef>
{
    const auto i = static_cast<usize>(h.index);
    if (i >= bodies_soa.size())
    {
        return std::nullopt;
    }
    return body(i);
}

auto PhysicsContext::try_body(BodyHandle h) const noexcept -> std::optional<BodyConstRef>
{
    const auto i = static_cast<usize>(h.index);
    if (i >= bodies_soa.size())
    {
        return std::nullopt;
    }
    return body(i);
}

auto PhysicsContext::body(usize i) noexcept -> BodyRef
{
    Expects(i < bodies_soa.size());
    return BodyRef{
        .id = bodies_soa.ids[i],
        .half_extents = bodies_soa.half_extents[i],
        .position = bodies_soa.positions[i],
        .velocity = bodies_soa.velocities[i],
        .force_accum = bodies_soa.force_accums[i],
        .inv_mass = bodies_soa.inv_masses[i],
        .orientation = bodies_soa.orientations[i],
        .angular_velocity = bodies_soa.angular_velocities[i],
        .torque_accum = bodies_soa.torque_accums[i],
        .inertia_body = bodies_soa.inertia_bodies[i],
        .inertia_world = bodies_soa.inertia_worlds[i],
        .inv_inertia_body = bodies_soa.inv_inertia_bodies[i],
        .inv_inertia_world = bodies_soa.inv_inertia_worlds[i],
        .asleep = BoolRef{bodies_soa.asleep_flags[i]},
        .sleep_frames = bodies_soa.sleep_frame_counts[i],
        .grabbed = BoolRef{bodies_soa.grabbed_flags[i]},
    };
}

auto PhysicsContext::body(usize i) const noexcept -> BodyConstRef
{
    Expects(i < bodies_soa.size());
    return BodyConstRef{
        .id = bodies_soa.ids[i],
        .half_extents = bodies_soa.half_extents[i],
        .position = bodies_soa.positions[i],
        .velocity = bodies_soa.velocities[i],
        .force_accum = bodies_soa.force_accums[i],
        .inv_mass = bodies_soa.inv_masses[i],
        .orientation = bodies_soa.orientations[i],
        .angular_velocity = bodies_soa.angular_velocities[i],
        .torque_accum = bodies_soa.torque_accums[i],
        .inertia_body = bodies_soa.inertia_bodies[i],
        .inertia_world = bodies_soa.inertia_worlds[i],
        .inv_inertia_body = bodies_soa.inv_inertia_bodies[i],
        .inv_inertia_world = bodies_soa.inv_inertia_worlds[i],
        .asleep = bodies_soa.asleep_flags[i] != 0u,
        .sleep_frames = bodies_soa.sleep_frame_counts[i],
        .grabbed = bodies_soa.grabbed_flags[i] != 0u,
    };
}

auto PhysicsContext::find_body_index(EntityId id) const noexcept -> std::optional<usize>
{
    for (auto i = 0zu; i < bodies_soa.size(); ++i)
    {
        if (bodies_soa.ids[i] == id)
        {
            return i;
        }
    }
    return std::nullopt;
}

auto PhysicsContext::body_count() const noexcept -> usize
{
    return bodies_soa.size();
}

auto PhysicsContext::set_debug_tracking_enabled(bool enabled) noexcept -> void
{
    debug_tracking_enabled = enabled;
    if (!debug_tracking_enabled)
    {
        clear_debug_tracking(*this);
    }
}

auto PhysicsContext::step() -> void
{
    contact_arena.clear();
    // How long the frame lasted, we do physics processing
    // until physics time reaches that time.
    const Duration dt{time_step};
    const f32 dt_s{dt_f32(dt)};

    update_inv_inertia_world(bodies_soa);

    parallel_for_index(
        bodies_soa.size(),
        [&](usize i) -> void
        {
            for (const auto& force : simple_forces)
            {
                apply_force(bodies_soa, i, force);
            }
        }
    );
    for (auto& force : complex_forces)
    {
        apply_force(bodies_soa, force);
    }

    integrate_forces(bodies_soa, dt_s);
    integrate_velocities(bodies_soa, dt_s);
    debug_collision_stats =
        generate_obb_contacts(bodies_soa, contact_arena, collision_scratch, debug_tracking_enabled);

    if (debug_tracking_enabled)
    {  // Setup debug info for this step
        debug_contacts.clear();
        debug_contacts.reserve(contact_arena.used() / sizeof(Contact));
        for (const auto& contact : contact_arena.as_span<Contact>())
        {
            debug_contacts.push_back(
                DebugContact{
                    .a_id = bodies_soa.ids[contact.a_idx],
                    .b_id = bodies_soa.ids[contact.b_idx],
                    .p = contact.p,
                    .n = contact.n,
                    .penetration = contact.penetration,
                    .allow_warm_start = contact.allow_warm_start,
                }
            );
        }
    }
    else
    {
        debug_contacts.clear();
        debug_collision_stats = {};
    }

    // Pull warm-start state from cache into contacts before warm start + solve.
    for (auto& contact : contact_arena.as_span<Contact>())
    {
        if (!contact.allow_warm_start)
        {
            continue;
        }
        const ContactKey key =
            make_contact_key(bodies_soa.ids[contact.a_idx], bodies_soa.ids[contact.b_idx], contact.p);
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
            warm_start_contact(bodies_soa, contact);
        }
    }

    solve_velocity_constraints(bodies_soa, contact_arena, dt_s);
    solve_position_constraints(bodies_soa, contact_arena);

    contact_cache.clear();
    contact_cache.reserve((contact_arena.used() / sizeof(Contact)) * 2zu);

    for (const auto& contact : contact_arena.as_span<Contact>())
    {
        if (!contact.allow_warm_start)
        {
            continue;
        }
        const ContactKey key =
            make_contact_key(bodies_soa.ids[contact.a_idx], bodies_soa.ids[contact.b_idx], contact.p);

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

    apply_sleep_and_damping(bodies_soa, dt_s);
    clear_accumulators(bodies_soa);

    if (debug_tracking_enabled)
    {
        debug_total_kinetic_energy = compute_total_kinetic_energy(bodies_soa, true);

        debug_energy_sample_accum += dt;
        while (debug_energy_sample_accum >= Duration{k_energy_sample_dt})
        {
            debug_energy_sample_accum -= Duration{k_energy_sample_dt};
            debug_total_kinetic_energy_history.push(debug_total_kinetic_energy);
        }
    }
    else
    {
        debug_total_kinetic_energy = 0.0f;
        debug_energy_sample_accum = Duration{0.0};
        debug_total_kinetic_energy_history.clear();
    }
    time = time + std::chrono::duration_cast<Clock::duration>(dt);
}

auto PhysicsContext::clear() -> void
{
    bodies_soa.clear();
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
