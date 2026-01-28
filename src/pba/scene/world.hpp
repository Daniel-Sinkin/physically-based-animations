// pba/scene/world.hpp
#pragma once

#include "pba/gfx/camera.hpp"
#include "pba/scene/entity.hpp"

namespace ds_pba
{
struct EditorState
{
    Camera camera{};
    std::vector<EntityId> selected_ids{};
    std::optional<EntityId> active_id{};

    void clear()
    {
        clear_selection();
        camera.pivot = k_camera_pivot;
    }

    [[nodiscard]] bool has_selection() const noexcept
    {
        return !selected_ids.empty();
    }

    [[nodiscard]] bool is_selected(EntityId id) const noexcept
    {
        return std::find(selected_ids.begin(), selected_ids.end(), id) != selected_ids.end();
    }

    void clear_selection() noexcept
    {
        selected_ids.clear();
        active_id.reset();
    }

    void select_single(EntityId id) noexcept
    {
        selected_ids.clear();
        selected_ids.push_back(id);
        active_id = id;
    }

    void toggle_selection(EntityId id) noexcept
    {
        auto it = std::find(selected_ids.begin(), selected_ids.end(), id);
        if (it != selected_ids.end())
        {
            selected_ids.erase(it);

            if (active_id && *active_id == id)
            {
                if (!selected_ids.empty())
                {
                    active_id = selected_ids.back();
                }
                else
                {
                    active_id.reset();
                }
            }
        }
        else
        {
            selected_ids.push_back(id);
            active_id = id;
        }

        if (selected_ids.empty())
        {
            active_id.reset();
        }
        else if (!active_id)
        {
            active_id = selected_ids.back();
        }
    }
};

struct World
{
    EditorState editor_state{};
    std::vector<Entity> cube_objects{};
    std::vector<Entity> sphere_objects{};
    std::vector<Entity> hitmarker_objects{};
    std::vector<Entity> marble_bust_objects{};

    void clear()
    {
        cube_objects.clear();
        sphere_objects.clear();
        hitmarker_objects.clear();
        marble_bust_objects.clear();
    }
};
}  // namespace ds_pba
