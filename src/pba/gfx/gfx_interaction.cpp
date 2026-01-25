// pba/gfx/gfx_interaction.cpp
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
#include "pba/engine/scene_context.hpp"
#include "pba/engine/scene_types.hpp"
#include "pba/gfx/raycast.hpp"
#include "pba/ui/ui.hpp"
//
#include <imgui.h>
#include <optional>
#include <utility>
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <json.hpp>

namespace ds_pba
{

void ds_pba::GfxContext::hover_interaction(const EditorInput& input) const
{
    const ImGuiIO& io{ImGui::GetIO()};

    Camera& cam{scene_context->camera};
    assert(viewport_fb_rect_valid && "If viewport hovered then it must be valid");
    const f32 wheel{io.MouseWheel};
    if (wheel != 0.0f)
    {  // Zooming
        cam.distance *= std::exp(-wheel * k_zoom_speed);
        cam.distance = std::clamp(scene_context->camera.distance, 0.75f, 200.0f);
    }

    if (input.mouse_middle.down)
    {
        hover_interaction_holding_middle(input, cam);
    }

    const bool selecting{input.mouse_left.pressed()};
    const bool spawning{input.mouse_right.pressed()};
    if (selecting || spawning)
    {  // Selecting objects
        const f32 aspect{viewport_fbo.aspect_ratio()};
        const ViewMatrix camera_view_matrix{scene_context->camera.view_matrix()};
        const ProjMatrix camera_proj_matrix{scene_context->camera.proj_matrix(aspect)};

        const glm::vec2 mouse_pos{
            static_cast<f32>(input.ui_mouse_x), static_cast<f32>(input.ui_mouse_y)
        };

        const Ray mouse_ray{ray_from_imgui_rect(
            mouse_pos, viewport_img_pos, viewport_img_size, camera_view_matrix, camera_proj_matrix
        )};

        const bool left_shift_down = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        const bool right_shift_down = glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        const bool shift_down{left_shift_down || right_shift_down};

        auto rc_res = raycast(*scene_context, mouse_ray);
        if (rc_res)
        {
            const Raycast rc = *rc_res;
            if (selecting)
            {
                hover_interaction_selection(input, rc);
            }
            if (spawning)
            {
                {
                    auto it = engine_context->obj_name_map.find(rc.object_id);
                    if (it != engine_context->obj_name_map.end())
                    {
                        ui_log(
                            std::format(
                                "Hit Object [id={} {}] at {} [distance from camera {:.2f}]",
                                rc.object_id,
                                it->second,
                                rc.hit,
                                rc.t
                            )
                        );
                    }
                    else
                    {
                        ui_log(
                            std::format(
                                "Hit Object [id={}] at {} [distance from camera {:.2f}]",
                                rc.object_id,
                                rc.hit,
                                rc.t
                            )
                        );
                    }
                }
                scene_context->hitmarker_objects.push_back(
                    Object{
                        .id = next_object_id(),
                        .type = ObjectType::Hitmarker,
                        .transform = {.position = rc.hit, .scale = {0.05f, 0.05f, 0.05f}},
                        .color = {1.0f, 1.0f, 1.0f},
                    }
                );
            }
        }
        else if (selecting && !shift_down)
        {
            // Deselect on clicking on background
            if (!scene_context->selected_ids.empty())
            {

                scene_context->clear_selection();
                ui_log("Deselected all");
            }
        }
    }
}

void ds_pba::GfxContext::hover_interaction_holding_middle(
    const EditorInput& input, Camera& cam
) const
{
    const auto dx = static_cast<f32>(input.ui_dx());
    const auto dy = static_cast<f32>(input.ui_dy());

    if (input.key_down(EditorKey::Shift))
    {  // Move Pivot
        const f32 vp_h{std::max(1.0f, viewport_img_size.y)};

        const f32 units_per_px{(2.0f * cam.distance * std::tan(0.5f * cam.fov_y)) / vp_h};

        auto right_offset = (-dx * units_per_px) * cam.right();
        auto up_offset = dy * units_per_px * cam.up();
        cam.pivot += (right_offset + up_offset) * k_pan_sensitivity;
    }
    else
    {  // Rotate Around pivot
        scene_context->camera.yaw += -dx * k_sensitivity;
        scene_context->camera.pitch += dy * k_sensitivity;

        const f32 lim{glm::radians(89.0f)};
        scene_context->camera.pitch = std::clamp(scene_context->camera.pitch, -lim, lim);
    }
}
void ds_pba::GfxContext::hover_interaction_selection(
    const EditorInput& input, const Raycast& rc
) const
{
    auto log_action = [&](std::string_view action, ObjectId id, const char* kind) -> void
    {
        if (engine_context)
        {
            if (auto it = engine_context->obj_name_map.find(id);
                it != engine_context->obj_name_map.end())
            {
                ui_log(std::format("{} {} [id={}] [{}]", action, it->second, id, kind));
                return;
            }
        }
        ui_log(std::format("{} [id={}] [{}]", action, id, kind));
    };

    const char* kind = "";
    switch (rc.object_type)
    {
        case ObjectType::Cube:
            kind = "Cube";
            break;
        case ObjectType::Sphere:
            kind = "Sphere";
            break;
        case ObjectType::Hitmarker:
            kind = "Hitmarker";
            break;
    }

    const bool was_selected = scene_context->is_selected(rc.object_id);

    if (input.key_down(EditorKey::Shift))
    {
        scene_context->toggle_selection(rc.object_id);
    }
    else
    {
        scene_context->select_single(rc.object_id);
    }

    const bool now_selected = scene_context->is_selected(rc.object_id);
    if (now_selected && !was_selected)
    {
        log_action("Selected", rc.object_id, kind);
    }
    else if (!now_selected && was_selected)
    {
        log_action("Deselected", rc.object_id, kind);
    }
    else
    {
        // e.g. clicking already selected without shift -> still selected
        log_action("Selected", rc.object_id, kind);
    }
}

}  // namespace ds_pba
