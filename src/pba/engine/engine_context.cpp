// pba/engine/engine_context.cpp
#include "glm/fwd.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine/engine_context.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/engine/scenes.hpp"
#include "pba/simulation.physics/physics_types.hpp"
#include "pba/ui/ui.hpp"
#include "pba/util/shutdown.hpp"
//
#include <algorithm>
#include <glm/gtc/quaternion.hpp>
#include <gsl/assert>

namespace ds_pba
{
namespace
{

[[nodiscard]] auto inv_inertia_body_box(f32 inv_mass, const Dir3& half_extents) noexcept
    -> glm::mat3
{
    if (inv_mass == k_static_mass)
    {
        return glm::mat3(0.0f);
    }

    Expects(inv_mass > 0.0f);
    if (inv_mass <= 0.0f)
    {
        return glm::mat3(0.0f);
    }
    const auto m = 1.0f / inv_mass;

    const auto x = 2.0f * half_extents.x;
    const auto y = 2.0f * half_extents.y;
    const auto z = 2.0f * half_extents.z;

    const auto Ixx = (m / 12.0f) * (y * y + z * z);
    const auto Iyy = (m / 12.0f) * (x * x + z * z);
    const auto Izz = (m / 12.0f) * (x * x + y * y);

    glm::mat3 invI{0.0f};
    invI[0][0] = (Ixx > 0.0f) ? (1.0f / Ixx) : 0.0f;
    invI[1][1] = (Iyy > 0.0f) ? (1.0f / Iyy) : 0.0f;
    invI[2][2] = (Izz > 0.0f) ? (1.0f / Izz) : 0.0f;
    return invI;
}

[[nodiscard]] auto
inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
    -> glm::mat3
{
    const glm::mat3 R{glm::mat3_cast(q)};
    return R * inv_inertia_body * glm::transpose(R);
}

auto init_box_inertia(RigidBody& b) noexcept -> void
{
    b.inv_inertia_body = inv_inertia_body_box(b.inv_mass, b.half_extents);
    b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
}

}  // namespace

auto EngineContext::setup() -> bool
{
    setup_active_scene();
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
