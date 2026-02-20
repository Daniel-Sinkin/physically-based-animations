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
#include "pba/scene/world.hpp"
#include "pba/ui/ui.hpp"
//
#include <glm/ext/matrix_float4x4.hpp>
#include <gsl/assert>
#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <print>

namespace ds_pba
{
namespace
{

constexpr f32 k_box_select_drag_threshold_px{6.0f};
constexpr f32 k_box_select_min_axis_px{1.0f};
constexpr f32 k_clip_w_epsilon{1.0e-5f};

struct ScreenRect
{
    f32 min_x;
    f32 min_y;
    f32 max_x;
    f32 max_y;
};

constexpr std::array<Pos3, 8> k_cube_local_corners{{
    {-0.5f, -0.5f, -0.5f},
    {-0.5f, -0.5f, +0.5f},
    {-0.5f, +0.5f, -0.5f},
    {-0.5f, +0.5f, +0.5f},
    {+0.5f, -0.5f, -0.5f},
    {+0.5f, -0.5f, +0.5f},
    {+0.5f, +0.5f, -0.5f},
    {+0.5f, +0.5f, +0.5f},
}};

constexpr std::array<Pos3, 9> k_cube_visibility_samples{{
    {0.0f, 0.0f, 0.0f},
    {-0.5f, -0.5f, -0.5f},
    {-0.5f, -0.5f, +0.5f},
    {-0.5f, +0.5f, -0.5f},
    {-0.5f, +0.5f, +0.5f},
    {+0.5f, -0.5f, -0.5f},
    {+0.5f, -0.5f, +0.5f},
    {+0.5f, +0.5f, -0.5f},
    {+0.5f, +0.5f, +0.5f},
}};

[[nodiscard]] auto make_screen_rect(const glm::vec2& a, const glm::vec2& b) noexcept -> ScreenRect
{
    return {
        .min_x = std::min(a.x, b.x),
        .min_y = std::min(a.y, b.y),
        .max_x = std::max(a.x, b.x),
        .max_y = std::max(a.y, b.y),
    };
}

[[nodiscard]] auto
clamp_screen_rect_to_viewport(const ScreenRect& rect, const glm::vec2& vp_pos, const glm::vec2& vp_size)
    noexcept -> ScreenRect
{
    const auto vp_min_x = vp_pos.x;
    const auto vp_min_y = vp_pos.y;
    const auto vp_max_x = vp_pos.x + std::max(1.0f, vp_size.x);
    const auto vp_max_y = vp_pos.y + std::max(1.0f, vp_size.y);

    ScreenRect out{
        .min_x = std::clamp(rect.min_x, vp_min_x, vp_max_x),
        .min_y = std::clamp(rect.min_y, vp_min_y, vp_max_y),
        .max_x = std::clamp(rect.max_x, vp_min_x, vp_max_x),
        .max_y = std::clamp(rect.max_y, vp_min_y, vp_max_y),
    };

    if (out.min_x > out.max_x)
    {
        std::swap(out.min_x, out.max_x);
    }
    if (out.min_y > out.max_y)
    {
        std::swap(out.min_y, out.max_y);
    }
    return out;
}

[[nodiscard]] auto screen_rect_intersects(const ScreenRect& a, const ScreenRect& b) noexcept -> bool
{
    return a.max_x >= b.min_x && a.min_x <= b.max_x && a.max_y >= b.min_y && a.min_y <= b.max_y;
}

[[nodiscard]] auto screen_rect_contains(const ScreenRect& rect, const glm::vec2& p) noexcept -> bool
{
    return p.x >= rect.min_x && p.x <= rect.max_x && p.y >= rect.min_y && p.y <= rect.max_y;
}

[[nodiscard]] auto project_world_to_screen(
    const Pos3& world_pos,
    const glm::mat4& view_proj,
    const glm::vec2& vp_pos,
    const glm::vec2& vp_size
) noexcept -> std::optional<glm::vec2>
{
    const auto clip = view_proj * glm::vec4(world_pos, 1.0f);
    if (clip.w <= k_clip_w_epsilon)
    {
        return std::nullopt;
    }

    const auto ndc = glm::vec3{clip} / clip.w;

    const auto x_01 = 0.5f * (ndc.x + 1.0f);
    const auto y_01 = 0.5f * (1.0f - ndc.y);
    const auto x = vp_pos.x + x_01 * vp_size.x;
    const auto y = vp_pos.y + y_01 * vp_size.y;
    return glm::vec2{x, y};
}

[[nodiscard]] auto cube_intersects_selection_rect(
    const ModelMatrix& model,
    const glm::mat4& view_proj,
    const glm::vec2& vp_pos,
    const glm::vec2& vp_size,
    const ScreenRect& selection_rect
) noexcept -> bool
{
    ScreenRect projected{
        .min_x = std::numeric_limits<f32>::max(),
        .min_y = std::numeric_limits<f32>::max(),
        .max_x = std::numeric_limits<f32>::lowest(),
        .max_y = std::numeric_limits<f32>::lowest(),
    };

    bool has_projected_corner{false};
    for (const auto& local_corner : k_cube_local_corners)
    {
        const auto world_corner = model.transform_position(local_corner);
        const auto screen_corner = project_world_to_screen(world_corner, view_proj, vp_pos, vp_size);
        if (!screen_corner)
        {
            continue;
        }

        has_projected_corner = true;
        projected.min_x = std::min(projected.min_x, screen_corner->x);
        projected.min_y = std::min(projected.min_y, screen_corner->y);
        projected.max_x = std::max(projected.max_x, screen_corner->x);
        projected.max_y = std::max(projected.max_y, screen_corner->y);
    }

    if (!has_projected_corner)
    {
        return false;
    }

    return screen_rect_intersects(projected, selection_rect);
}

[[nodiscard]] auto gather_box_selection_ids(
    const World& world,
    const PhysicsContext& physics,
    const ViewMatrix& view,
    const ProjMatrix& proj,
    const glm::vec2& vp_pos,
    const glm::vec2& vp_size,
    const ScreenRect& selection_rect
) -> std::vector<EntityId>
{
    const auto view_proj = proj.m * view.m;
    std::vector<EntityId> ids{};

    const auto entities = world.entities();
    ids.reserve(entities.size());

    for (auto i = 0zu; i < entities.size(); ++i)
    {
        const auto& entity = entities[i];
        if (entity.type != EntityType::Cube)
        {
            continue;
        }

        if (entity.body)
        {
            const auto rb = physics.try_body(*entity.body);
            if (rb && rb->is_static())
            {
                continue;
            }
        }

        const auto model = world.model_matrix_at(i);
        if (!model)
        {
            continue;
        }

        if (cube_intersects_selection_rect(*model, view_proj, vp_pos, vp_size, selection_rect))
        {
            ids.push_back(entity.id);
        }
    }

    return ids;
}

[[nodiscard]] auto is_cube_visible_in_rect(
    const World& world,
    const EntityId id,
    const ModelMatrix& model,
    const glm::mat4& view_proj,
    const ViewMatrix& view,
    const ProjMatrix& proj,
    const glm::vec2& vp_pos,
    const glm::vec2& vp_size,
    const ScreenRect& selection_rect
) -> bool
{
    for (const auto& local_sample : k_cube_visibility_samples)
    {
        const auto world_sample = model.transform_position(local_sample);
        const auto screen_pos = project_world_to_screen(world_sample, view_proj, vp_pos, vp_size);
        if (!screen_pos || !screen_rect_contains(selection_rect, *screen_pos))
        {
            continue;
        }

        const auto sample_ray = ray_from_imgui_rect(*screen_pos, vp_pos, vp_size, view, proj);
        const auto sample_hit = raycast(world, sample_ray);
        if (sample_hit && sample_hit->object_id == id)
        {
            return true;
        }
    }
    return false;
}

[[nodiscard]] auto filter_visible_box_selection_ids(
    const World& world,
    std::span<const EntityId> candidates,
    const ViewMatrix& view,
    const ProjMatrix& proj,
    const glm::vec2& vp_pos,
    const glm::vec2& vp_size,
    const ScreenRect& selection_rect
) -> std::vector<EntityId>
{
    const auto view_proj = proj.m * view.m;
    std::vector<EntityId> visible{};
    visible.reserve(candidates.size());

    for (const auto id : candidates)
    {
        const auto idx = world.find_idx(id);
        if (!idx)
        {
            continue;
        }

        const auto model = world.model_matrix_at(*idx);
        if (!model)
        {
            continue;
        }

        if (is_cube_visible_in_rect(
                world,
                id,
                *model,
                view_proj,
                view,
                proj,
                vp_pos,
                vp_size,
                selection_rect
            ))
        {
            visible.push_back(id);
        }
    }
    return visible;
}

[[nodiscard]] auto plural_suffix(usize n) noexcept -> const char*
{
    return n == 1zu ? "" : "s";
}

}  // namespace

auto GfxContext::hover_interaction(EditorInput& input) -> void
{
    {
        Expects(viewport_fb_rect_valid && "If viewport hovered then it must be valid");
    }
    const auto& imgui_io = ImGui::GetIO();

    auto& world = engine_context->simulation.world;
    auto& world_editor_state = world.editor_state();
    auto& cam = world_editor_state.camera();

    if (viewport_image_hovered)
    {
        const f32 wheel{imgui_io.MouseWheel};
        if (wheel != 0.0f)
        {  // Zooming
            cam.distance *= std::exp(-wheel * k_zoom_speed);
            cam.distance = std::clamp(cam.distance, 0.75f, 200.0f);
        }

        if (input.mouse_middle.down || input.key_down(EditorKey::R))
        {
            hover_interaction_holding_middle(input, cam);
        }
    }

    auto& box_select = editor.box_select;
    if (input.mouse_left.pressed() && viewport_image_hovered)
    {
        box_select.active = true;
        box_select.dragging = false;
        box_select.start_mouse_x = input.ui_mouse_x;
        box_select.start_mouse_y = input.ui_mouse_y;
        box_select.current_mouse_x = input.ui_mouse_x;
        box_select.current_mouse_y = input.ui_mouse_y;
    }

    if (!box_select.active)
    {
        return;
    }

    box_select.current_mouse_x = input.ui_mouse_x;
    box_select.current_mouse_y = input.ui_mouse_y;

    const auto drag_dx = narrow_cast<f32>(box_select.current_mouse_x - box_select.start_mouse_x);
    const auto drag_dy = narrow_cast<f32>(box_select.current_mouse_y - box_select.start_mouse_y);
    if (!box_select.dragging)
    {
        const auto drag_d2 = drag_dx * drag_dx + drag_dy * drag_dy;
        const auto threshold_d2 = k_box_select_drag_threshold_px * k_box_select_drag_threshold_px;
        box_select.dragging = drag_d2 >= threshold_d2;
    }

    if (!input.mouse_left.released())
    {
        return;
    }

    const auto aspect = viewport_fbo.aspect_ratio();
    const auto camera_view_matrix = cam.view_matrix();
    const auto camera_proj_matrix = cam.proj_matrix(aspect);

    if (!box_select.dragging)
    {
        if (viewport_image_hovered)
        {  // Point selection on click release.
            const glm::vec2 mouse_pos{input.ui_mouse_x, input.ui_mouse_y};
            const Ray mouse_ray{ray_from_imgui_rect(
                mouse_pos,
                viewport_img_pos,
                viewport_img_size,
                camera_view_matrix,
                camera_proj_matrix
            )};

            if (const auto rc_res = raycast(world, mouse_ray); rc_res)
            {
                hover_interaction_selection(input, *rc_res);
            }
            else if (!world_editor_state.selected_ids.empty() && !input.key_down(EditorKey::Shift))
            {
                world_editor_state.clear_selection();
                ui_log("Deselected all");
            }
        }

        box_select.active = false;
        box_select.dragging = false;
        return;
    }

    const ScreenRect drag_rect = clamp_screen_rect_to_viewport(
        make_screen_rect(
            {narrow_cast<f32>(box_select.start_mouse_x), narrow_cast<f32>(box_select.start_mouse_y)},
            {
                narrow_cast<f32>(box_select.current_mouse_x),
                narrow_cast<f32>(box_select.current_mouse_y),
            }
        ),
        viewport_img_pos,
        viewport_img_size
    );

    const auto drag_w = drag_rect.max_x - drag_rect.min_x;
    const auto drag_h = drag_rect.max_y - drag_rect.min_y;
    if (drag_w < k_box_select_min_axis_px && drag_h < k_box_select_min_axis_px)
    {
        box_select.active = false;
        box_select.dragging = false;
        return;
    }

    const bool additive = input.key_down(EditorKey::Shift);
    const bool had_selection = !world_editor_state.selected_ids.empty();
    auto ids_in_drag = gather_box_selection_ids(
        world,
        engine_context->simulation.physics,
        camera_view_matrix,
        camera_proj_matrix,
        viewport_img_pos,
        viewport_img_size,
        drag_rect
    );
    const bool through_depth = imgui_io.KeyCtrl || imgui_io.KeySuper;
    if (!through_depth)
    {
        ids_in_drag = filter_visible_box_selection_ids(
            world,
            ids_in_drag,
            camera_view_matrix,
            camera_proj_matrix,
            viewport_img_pos,
            viewport_img_size,
            drag_rect
        );
    }

    usize added_count{0zu};
    if (!additive)
    {
        world_editor_state.selected_ids = ids_in_drag;
        if (world_editor_state.selected_ids.empty())
        {
            world_editor_state.active_id.reset();
        }
        else
        {
            world_editor_state.active_id = world_editor_state.selected_ids.back();
        }
        added_count = world_editor_state.selected_ids.size();
    }
    else
    {
        for (const auto id : ids_in_drag)
        {
            if (!world_editor_state.is_selected(id))
            {
                world_editor_state.toggle_selection(id);
                ++added_count;
            }
        }
    }

    if (added_count > 0zu)
    {
        if (additive)
        {
            ui_log(
                std::format(
                    "Added {} cube{} to selection", added_count, plural_suffix(added_count)
                )
            );
        }
        else
        {
            ui_log(std::format("Selected {} cube{} (box)", added_count, plural_suffix(added_count)));
        }
    }
    else if (!additive && had_selection)
    {
        ui_log("Deselected all");
    }

    box_select.active = false;
    box_select.dragging = false;
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
