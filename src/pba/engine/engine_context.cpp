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
auto EngineContext::setup() -> bool
{
    setup_active_scene(simulation);
    accumulator = Duration{0.0};
    frame_time = Clock::now();
    simulation.physics.time = frame_time;

    gfx.engine_context = this;
    if (!gfx.setup())
    {
        return false;
    }

    frame_time = Clock::now();
    simulation.physics.time = frame_time;
    accumulator = Duration{0.0};

    return true;
}

auto EngineContext::run() -> void
{
    const Duration fixed_dt{simulation.physics.time_step};
    const Duration max_frame_dt{0.25};

    frame_time = Clock::now();
    simulation.physics.time = frame_time;
    accumulator = Duration{0.0};

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
            paused = !paused;
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
                accumulator = Duration{0.0};
                frame_time = Clock::now();
                simulation.physics.time = frame_time;
            }
            prev_paused = paused;
        }

        const TimePoint now{Clock::now()};
        Duration frame_dt{std::chrono::duration_cast<Duration>(now - frame_time)};
        frame_time = now;
        frame_dt = std::min(frame_dt, max_frame_dt);

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
