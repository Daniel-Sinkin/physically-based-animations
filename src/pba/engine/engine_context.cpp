// pba/engine/engine_context.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine/engine_context.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/physics/physics_types.hpp"
#include "pba/simulation/scenes.hpp"
#include "pba/ui/ui.hpp"
#include "pba/util/shutdown.hpp"
//
#include "glm/fwd.hpp"
//
#include <algorithm>
#include <glm/gtc/quaternion.hpp>
#include <gsl/assert>

namespace ds_pba
{
auto EngineContext::reset_simulation_clock() noexcept -> void
{
    accumulator = Duration{0.0};
    frame_time = Clock::now();
    simulation.physics.time = frame_time;
}

auto EngineContext::set_paused(bool is_paused) noexcept -> void
{
    paused = is_paused;
    if (!paused)
    {
        reset_simulation_clock();
    }
}

auto EngineContext::maybe_emit_cube(const TimePoint& now) -> void
{
    if (gfx.editor.grab.active || gfx.imgui_uses_keyboard || !gfx.viewport_image_hovered
        || !editor_input.key_down(EditorKey::F))
    {
        spit_cube.has_last_emit = false;
        return;
    }

    const auto spawn_interval = Duration{k_emitter_spawn_interval_s};
    if (spit_cube.has_last_emit && (now - spit_cube.last_emit) < spawn_interval)
    {
        return;
    }

    spit_cube.last_emit = now;
    spit_cube.has_last_emit = true;

    auto& cam = simulation.world.editor_state().camera();

    const auto cam_pos = cam.position();
    auto forward = cam.pivot - cam_pos;
    const auto len2 = glm::dot(forward, forward);
    if (len2 > 1e-10f)
    {
        forward /= std::sqrt(len2);
    }
    else
    {
        forward = k_axis_x;
    }

    const auto spawn_pos = cam_pos + k_emitter_spawn_offset * forward;
    const auto vel = k_emitter_launch_speed * forward;

    auto& e = simulation.spawn_cube(
        spawn_pos,
        Dir3{0.5f, 0.5f, 0.5f},
        1.0f,
        vel,
        k_quaternion_identity,
        k_zero_dir,
        Color3{0.93f, 0.93f, 0.98f},
        "Spit Cube"
    );
    if (!e.body)
    {
        return;
    }

    auto rb = simulation.physics.try_body(*e.body);
    if (!rb)
    {
        return;
    }

    rb->grabbed = true;
    rb->asleep = false;
    rb->velocity = vel;

    const auto disable_for =
        std::chrono::duration_cast<Clock::duration>(Duration{k_emitter_disable_physics_s});
    spit_cube.pending.push_back(SpitCubePending{
        .body = *e.body,
        .reenable_time = now + disable_for,
    });
}

auto EngineContext::update_pending_spit_cubes(const TimePoint& now, Duration frame_dt) -> void
{
    auto write = 0zu;
    const auto dt_s = static_cast<f32>(frame_dt.count());

    for (auto i = 0zu; i < spit_cube.pending.size(); ++i)
    {
        const auto pending = spit_cube.pending[i];
        auto rb = simulation.physics.try_body(pending.body);
        if (!rb)
        {
            continue;
        }

        if (rb->grabbed)
        {
            rb->position += rb->velocity * dt_s;
            (void) simulation.world.set_position(rb->id, rb->position);
        }

        if (now >= pending.reenable_time)
        {
            rb->grabbed = false;
            continue;
        }

        spit_cube.pending[write++] = pending;
    }
    spit_cube.pending.resize(write);
}

auto EngineContext::setup() -> bool
{
    setup_active_scene(simulation);
    reset_simulation_clock();
    spit_cube.pending.clear();
    spit_cube.has_last_emit = false;

    gfx.engine_context = this;
    if (!gfx.setup())
    {
        return false;
    }

    reset_simulation_clock();

    return true;
}

auto EngineContext::run() -> void
{
    const Duration fixed_dt{simulation.physics.time_step};
    const Duration max_frame_dt{0.25};

    reset_simulation_clock();

    bool prev_paused{paused};

    while (is_active())
    {
        {  // See shutdown.hpp for details on our signal handling
            if (g_request_close_sig)
            {
                g_request_close.store(true, std::memory_order_relaxed);
                g_request_close_sig = 0;
            }
            if (g_request_close.load(std::memory_order_relaxed))
            {
                request_close();
            }
        }

        glfwPollEvents();
        editor_input.update(not_null{gfx.window});
        gfx.step(editor_input);

        if (!gfx.imgui_uses_keyboard && editor_input.key_pressed(EditorKey::Space))
        {
            set_paused(!paused);
        }

        if (paused != prev_paused)
        {
            if (paused)
            {
                ui_log("Paused (SPACE to resume)");
            }
            else
            {
                ui_log("Running (SPACE to pause)");
            }
            prev_paused = paused;
        }

        const TimePoint now{Clock::now()};
        Duration frame_dt{std::chrono::duration_cast<Duration>(now - frame_time)};
        frame_time = now;
        frame_dt = std::min(frame_dt, max_frame_dt);

        update_pending_spit_cubes(now, frame_dt);
        maybe_emit_cube(now);

        if (paused)
        {
            continue;
        }

        accumulator += frame_dt;
        while (accumulator >= fixed_dt)
        {
            simulation.physics.step();
            accumulator -= fixed_dt;
        }

        simulation.sync_physics_to_world();
    }
}

}  // namespace ds_pba
