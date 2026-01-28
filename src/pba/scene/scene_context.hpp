// pba/scene/scene_context.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/gfx/camera.hpp"

//
#include <algorithm>
#include <optional>
#include <vector>

namespace ds_pba
{
struct SceneContext
{
    Camera camera{};
    std::vector<Entity> cube_objects{};
    std::vector<Entity> sphere_objects{};
    std::vector<Entity> hitmarker_objects{};
    std::vector<Entity> marble_bust_objects{};

    std::vector<EntityId> selected_ids{};
    std::optional<EntityId> active_id{};

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

}  // namespace ds_pba
