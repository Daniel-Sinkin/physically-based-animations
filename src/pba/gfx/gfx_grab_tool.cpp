// pba/gfx/gfx_grab_tool.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/engine/scene_context.hpp"
#include "pba/gfx/raycast.hpp"
#include "pba/ui/ui.hpp"
//
#include <imgui.h>
#include <optional>
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <json.hpp>

namespace ds_pba
{

void ds_pba::GfxContext::cancel_grab()
{
    if (!grab.active)
    {
        return;
    }

    for (const auto& [id, start_pos] : grab.start_positions)
    {
        set_object_position(id, start_pos);
    }

    grab.active = false;
    grab.start_positions.clear();
    grab.constraint = GrabConstraint::None;
    ui_log("Grab cancelled");
}

void ds_pba::GfxContext::confirm_grab()
{
    if (!grab.active)
    {
        return;
    }

    grab.active = false;
    grab.start_positions.clear();
    grab.constraint = GrabConstraint::None;
    ui_log("Grab confirmed");
}
void ds_pba::GfxContext::update_grab(f64 mouse_x, f64 mouse_y)
{
    assert(scene_context);

    if (!grab.active)
    {
        return;
    }

    const Camera& cam{scene_context->camera};

    const f32 vp_h{std::max(1.0f, viewport_img_size.y)};
    const f32 units_per_px{(2.0f * cam.distance * std::tan(0.5f * cam.fov_y)) / vp_h};

    const auto dx = static_cast<f32>(mouse_x - grab.start_mouse_x);
    const auto dy = static_cast<f32>(mouse_y - grab.start_mouse_y);

    // Mouse up = Look down
    const Direction3 v = (dx * units_per_px) * cam.right() + (-dy * units_per_px) * cam.up();

    Direction3 delta = v;

    switch (grab.constraint)
    {
        case GrabConstraint::None:
            break;

        case GrabConstraint::X:
            delta = Direction3{delta.x, 0.0f, 0.0f};
            break;

        case GrabConstraint::Y:
            delta = Direction3{0.0f, delta.y, 0.0f};
            break;

        case GrabConstraint::Z:
            delta = Direction3{0.0f, 0.0f, delta.z};
            break;
    }

    for (const auto& [id, start_pos] : grab.start_positions)
    {
        set_object_position(id, start_pos + delta);
    }
}
void ds_pba::GfxContext::set_grab_constraint(GrabConstraint c)
{
    grab.constraint = c;

    switch (c)
    {
        case GrabConstraint::None:
            ui_log("Grab: unconstrained");
            break;
        case GrabConstraint::X:
            ui_log("Grab: constrain X");
            break;
        case GrabConstraint::Y:
            ui_log("Grab: constrain Y");
            break;
        case GrabConstraint::Z:
            ui_log("Grab: constrain Z");
            break;
    }
}

void ds_pba::GfxContext::begin_grab(f64 mouse_x, f64 mouse_y)
{
    assert(scene_context);

    if (scene_context->selected_ids.empty())
    {
        ui_log("Grab (G): nothing selected");
        return;
    }

    grab.active = true;
    grab.start_mouse_x = mouse_x;
    grab.start_mouse_y = mouse_y;
    grab.constraint = GrabConstraint::None;

    grab.start_positions.clear();
    grab.start_positions.reserve(scene_context->selected_ids.size());

    for (const ObjectId id : scene_context->selected_ids)
    {
        if (auto p = get_object_position(id))
        {
            grab.start_positions.emplace_back(id, *p);
        }
    }

    ui_log("Grab: move mouse. X/Y/Z constrain. LMB/Enter confirm. RMB/Esc cancel.");
}
}  // namespace ds_pba
