// pba/gfx/gfx_grab_tool.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/engine/engine_context.hpp"
#include "pba/ui/ui.hpp"

namespace ds_pba
{
namespace
{
[[nodiscard]] GfxContext::EditorState::GrabConstraint
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

void set_entity_and_body_position(EngineContext& e, EntityId id, const Pos3& pos) noexcept
{
    if (auto* entity = e.world.find(id))
    {
        entity->transform.position = pos;

        if (entity->body)
        {
            if (auto* rb = e.physics.try_body(*entity->body))
            {
                rb->position = pos;
            }
        }
    }
}

}  // namespace

void GfxContext::cancel_grab()
{
    Expects(engine_context);
    Expects(editor.grab.active);
    if (!editor.grab.active)
    {
        return;
    }

    for (const auto& [id, start_pos] : editor.grab.start_positions)
    {
        find(auto entity_res = engine_context->world.find(id); entity_res)
        {
            *entity_res.grabbed = false;
        }
        set_entity_and_body_position(*engine_context, id, start_pos);
    }

    editor.grab.active = false;
    editor.grab.start_positions.clear();
    editor.grab.constraint = EditorState::GrabConstraint::None;

    ui_log("Grab cancelled");
}

void GfxContext::confirm_grab()
{
    auto& grab = editor.grab;
    if (!grab.active)
    {
        return;
    }

    Expects(engine_context);

    // During dragging we update Entity::transform.position.
    // Confirm must synchronize physics bodies to the final transform positions.
    for (const auto& [id, _start_pos] : grab.start_positions)
    {
        if (auto* entity = engine_context->world.find(id))
        {
            if (entity->body)
            {
                if (auto* rb = engine_context->physics.try_body(*entity->body))
                {
                    rb->position = entity->transform.position;
                    rb->grabbed = false;
                }
            }
        }
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
    Expects(engine_context);

    const auto& selected_ids = engine_context->world.editor_state().selected_ids;
    if (selected_ids.empty())
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
    grab.start_positions.reserve(selected_ids.size());

    for (const EntityId id : selected_ids)
    {
        if (auto* entity = engine_context->world.find(id))
        {
            Pos3 start_pos = entity->transform.position;

            if (entity->body)
            {
                if (auto* rb = engine_context->physics.try_body(*entity->body))
                {
                    rb->grabbed = true;
                    rb->asleep = false;
                    rb->sleep_frames = 0;
                    rb->velocity = Dir3{};
                    rb->angular_velocity = Dir3{};
                    start_pos = rb->position;
                    entity->transform.position = start_pos;
                }
            }

            grab.start_positions.emplace_back(id, start_pos);
        }
    }

    ui_log("Grab: move mouse. X/Y/Z constrain. LMB/Enter confirm. RMB/Esc cancel.");
}

void GfxContext::update_grab(const EditorInput& input)
{
    auto& grab = editor.grab;
    if (!grab.active)
    {
        return;
    }

    Expects(engine_context);

    if (const auto c = grab_constraint_from_key(input); c != EditorState::GrabConstraint::None)
    {
        set_grab_constraint(c);
    }

    const auto& cam = engine_context->world.editor_state().camera();

    const auto vp_h = std::max(1.0f, viewport_img_size.y);
    const auto units_per_px = (2.0f * cam.distance * std::tan(0.5f * cam.fov_y)) / vp_h;

    const auto dx = narrow_cast<f32>(input.ui_mouse_x - grab.start_mouse_x);
    const auto dy = narrow_cast<f32>(input.ui_mouse_y - grab.start_mouse_y);

    // Mouse up = look down
    Dir3 delta{(dx * units_per_px) * cam.right() + (-dy * units_per_px) * cam.up()};

    using GC = EditorState::GrabConstraint;
    switch (grab.constraint)
    {
        case GC::None:
            break;
        case GC::X:
            delta = Dir3{delta.x, 0.0f, 0.0f};
            break;
        case GC::Y:
            delta = Dir3{0.0f, delta.y, 0.0f};
            break;
        case GC::Z:
            delta = Dir3{0.0f, 0.0f, delta.z};
            break;
    }

    for (const auto& [id, start_pos] : grab.start_positions)
    {
        set_entity_and_body_position(*engine_context, id, start_pos + delta);
    }
}

}  // namespace ds_pba
