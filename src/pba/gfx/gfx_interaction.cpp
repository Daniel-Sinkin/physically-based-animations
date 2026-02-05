// pba/gfx/gfx_interaction.cpp
#include "imgui.h"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/editor/editor_input.hpp"
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/engine/engine_context.hpp"
#include "pba/gfx/raycast.hpp"
#include "pba/scene/entity.hpp"
#include "pba/ui/ui.hpp"
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <gsl/assert>
#include <json.hpp>
#include <print>

namespace ds_pba
{

auto GfxContext::hover_interaction(EditorInput& input) const -> void
{
    {
        Expects(viewport_fb_rect_valid && "If viewport hovered then it must be valid");
    }
    const auto& imgui_io = ImGui::GetIO();

    auto& cam{engine_context->simulation.world.editor_state().camera()};
    const f32 wheel{imgui_io.MouseWheel};
    if (wheel != 0.0f)
    {  // Zooming
        cam.distance *= std::exp(-wheel * k_zoom_speed);
        cam.distance = std::clamp(
            engine_context->simulation.world.editor_state().camera().distance, 0.75f, 200.0f
        );
    }

    if (input.mouse_middle.down || input.key_down(EditorKey::R))
    {
        hover_interaction_holding_middle(input, cam);
    }

    const bool selecting{input.mouse_left.pressed()};
    if (selecting)
    {  // Selecting objects
        const auto aspect = viewport_fbo.aspect_ratio();
        const auto camera_view_matrix =
            engine_context->simulation.world.editor_state().camera().view_matrix();
        const auto camera_proj_matrix =
            engine_context->simulation.world.editor_state().camera().proj_matrix(aspect);

        const glm::vec2 mouse_pos{input.ui_mouse_x, input.ui_mouse_y};

        const Ray mouse_ray{ray_from_imgui_rect(
            mouse_pos, viewport_img_pos, viewport_img_size, camera_view_matrix, camera_proj_matrix
        )};

        auto rc_res = raycast(engine_context->simulation.world, mouse_ray);
        if (rc_res)
        {
            const Raycast rc{*rc_res};
            if (selecting)
            {
                hover_interaction_selection(input, rc);
            }
        }
        else if (!engine_context->simulation.world.editor_state().selected_ids.empty()
                 && !input.key_down(EditorKey::Shift))
        {
            // Deselect on clicking on background
            if (!engine_context->simulation.world.editor_state().selected_ids.empty())
            {
                engine_context->simulation.world.editor_state().clear_selection();
                ui_log("Deselected all");
            }
        }
    }
}

auto GfxContext::hover_interaction_holding_middle(EditorInput& input, Camera& cam) const -> void
{
    const auto dx = input.ui_dx();
    const auto dy = input.ui_dy();

    if (input.key_down(EditorKey::Shift))
    {  // Move Pivot
        const auto pan_world = cam.pan_offset_world(dx, dy, viewport_img_size.y);
        cam.pivot += pan_world * k_pan_sensitivity;
    }
    else
    {  // Rotate Around pivot
        constexpr bool k_invert_mouse_pivot_y{false};

        cam.yaw += -dx * k_sensitivity;
        cam.pitch += (k_invert_mouse_pivot_y ? -1.0f : 1.0f) * dy * k_sensitivity;

        const auto pitch_lim = glm::radians(k_camera_pitch_lim_deg);
        cam.pitch = std::clamp(cam.pitch, -pitch_lim, pitch_lim);
    }
}

auto GfxContext::hover_interaction_selection(EditorInput& input, const Raycast& rc) const -> void
{
    Expects(engine_context);
    if (!engine_context)
    {
        std::println("Engine Context ist not initialised, cna do hover_interaction_selection");
        return;
    }
    auto log_action = [&](std::string_view action, EntityId id, const char* kind) -> void
    {
        if (auto entity = engine_context->simulation.world.find(id); entity)
        {
            ui_log(std::format("{} {} [id={}] [{}]", action, entity->name, id, kind));
        }
        else
        {
            ui_log(std::format("{} [id={}] [{}]", action, id, kind));
        }
    };

    const bool was_selected =
        engine_context->simulation.world.editor_state().is_selected(rc.object_id);

    if (input.key_down(EditorKey::Shift))
    {
        engine_context->simulation.world.editor_state().toggle_selection(rc.object_id);
        const bool now_selected =
            engine_context->simulation.world.editor_state().is_selected(rc.object_id);
        if (now_selected)
        {
            log_action("Selected", rc.object_id, "Cube");
        }
        else
        {
            log_action("Deselected", rc.object_id, "Cube");
        }
        return;
    }

    if (was_selected)
    {
        engine_context->simulation.world.editor_state().toggle_selection(rc.object_id);
        log_action("Deselected", rc.object_id, "Cube");
        return;
    }

    engine_context->simulation.world.editor_state().select_single(rc.object_id);
    log_action("Selected", rc.object_id, "Cube");
}

}  // namespace ds_pba
