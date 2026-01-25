// pba/gfx/gfx_grab_tool.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/engine/scene_context.hpp"
#include "pba/ui/ui.hpp"
//
#include <optional>

namespace ds_pba
{
namespace
{
[[nodiscard]] static GfxContext::EditorState::GrabConstraint
grab_constraint_from_key(const EditorInput& in) noexcept
{
    using GC = GfxContext::EditorState::GrabConstraint;

    if (in.key_pressed(EditorKey::X))
    {
        return GC::X;
    }

    if (in.key_pressed(EditorKey::Y))
    {
        return k_is_german_keyboard ? GC::Z : GC::Y;
    }

    if (in.key_pressed(EditorKey::Z))
    {
        return k_is_german_keyboard ? GC::Y : GC::Z;
    }

    return GC::None;
}
}  // namespace

void GfxContext::cancel_grab()
{
    auto& grab = editor.grab;
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
    grab.constraint = EditorState::GrabConstraint::None;
    ui_log("Grab cancelled");
}

void GfxContext::confirm_grab()
{
    auto& grab = editor.grab;
    if (!grab.active)
    {
        return;
    }

    grab.active = false;
    grab.start_positions.clear();
    grab.constraint = EditorState::GrabConstraint::None;
    ui_log("Grab confirmed");
}

void GfxContext::set_grab_constraint(EditorState::GrabConstraint c)
{
    auto& grab = editor.grab;
    grab.constraint = c;

    using GC = EditorState::GrabConstraint;
    switch (c)
    {
        case GC::None:
            ui_log("Grab: unconstrained");
            break;
        case GC::X:
            ui_log("Grab: constrain X");
            break;
        case GC::Y:
            ui_log("Grab: constrain Y");
            break;
        case GC::Z:
            ui_log("Grab: constrain Z");
            break;
    }
}

void GfxContext::begin_grab(const EditorInput& input)
{
    assert(scene_context);

    if (scene_context->selected_ids.empty())
    {
        ui_log("Grab (G): nothing selected");
        return;
    }

    auto& grab = editor.grab;

    grab.active = true;
    grab.start_mouse_x = input.ui_mouse_x;
    grab.start_mouse_y = input.ui_mouse_y;
    grab.constraint = EditorState::GrabConstraint::None;

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

void GfxContext::update_grab(const EditorInput& input)
{
    assert(scene_context);

    auto& grab = editor.grab;
    if (!grab.active)
    {
        return;
    }

    if (const auto c = grab_constraint_from_key(input); c != EditorState::GrabConstraint::None)
    {
        set_grab_constraint(c);
    }

    const Camera& cam{scene_context->camera};

    const f32 vp_h{std::max(1.0f, viewport_img_size.y)};
    const f32 units_per_px{(2.0f * cam.distance * std::tan(0.5f * cam.fov_y)) / vp_h};

    const f32 dx{static_cast<f32>(input.ui_mouse_x - grab.start_mouse_x)};
    const f32 dy{static_cast<f32>(input.ui_mouse_y - grab.start_mouse_y)};

    // Mouse up = look down
    const Direction3 v{(dx * units_per_px) * cam.right() + (-dy * units_per_px) * cam.up()};
    Direction3 delta{v};

    using GC = EditorState::GrabConstraint;
    switch (grab.constraint)
    {
        case GC::None:
            break;
        case GC::X:
            delta = Direction3{delta.x, 0.0f, 0.0f};
            break;
        case GC::Y:
            delta = Direction3{0.0f, delta.y, 0.0f};
            break;
        case GC::Z:
            delta = Direction3{0.0f, 0.0f, delta.z};
            break;
    }

    for (const auto& [id, start_pos] : grab.start_positions)
    {
        set_object_position(id, start_pos + delta);
    }
}

}  // namespace ds_pba
