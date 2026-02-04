// pba/engine/engine_context.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/gfx/gfx_context.hpp"
#include "pba/physics/physics_context.hpp"
#include "pba/physics/physics_types.hpp"
#include "pba/scene/entity.hpp"
#include "pba/scene/world.hpp"
#include "pba/simulation/scene_id.hpp"
#include "pba/simulation/simulation_context.hpp"

#include <gsl/assert>

namespace ds_pba
{
struct EngineContext
{
    GfxContext gfx{};
    SimulationContext simulation{};

    EditorInput editor_input{};

    [[nodiscard]] auto setup() -> bool;
    struct EntityLink
    {
        usize cube_obj_idx;
        usize physics_obj_idx;
    };

    TimePoint frame_time = Clock::now();
    Duration accumulator{};
    bool paused{true};

    auto run() -> void;
    auto is_active() const -> bool
    {
        return (gfx.window != nullptr) && !glfwWindowShouldClose(gfx.window);
    }

    void request_close() noexcept
    {
        is_active_ = false;
        if (gfx.window)
        {
            glfwSetWindowShouldClose(gfx.window, GLFW_TRUE);
        }
    }
    bool is_active_{true};

    auto deactivate() noexcept -> void
    {
        is_active_ = false;
    }
    auto activate() noexcept -> void
    {
        is_active_ = true;
    }
};

}  // namespace ds_pba
