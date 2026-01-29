// pba/scene/world.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/engine/engine_context.hpp"
#include "pba/scene/camera.hpp"
#include "pba/scene/entity.hpp"

#include <exception>
#include <gsl/assert>

namespace ds_pba
{
struct EngineContext;

class EditorState
{
  public:
    Camera& camera()
    {
        return camera_;
    }

    const Camera& camera() const
    {
        return camera_;
    }

    std::vector<EntityId> selected_ids{};
    std::optional<EntityId> active_id{};

    void clear() noexcept
    {
        clear_selection();
        camera_.pivot = k_camera_pivot;
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

    void erase_from_selection(EntityId id) noexcept
    {
        auto it = std::find(selected_ids.begin(), selected_ids.end(), id);
        if (it != selected_ids.end())
        {
            selected_ids.erase(it);
        }
        if (active_id && *active_id == id)
        {
            active_id.reset();
            if (!selected_ids.empty())
            {
                active_id = selected_ids.back();
            }
        }
    }

  private:
    Camera camera_{};
};
class World
{
  public:
    World() = default;

    void clear(bool reset_ids = true) noexcept
    {
        editor_state_.clear();

        entities_.clear();
        id_to_index_.clear();

        if (reset_ids)
        {
            next_id_ = 0u;
        }
    }

    Entity& spawn(EntityType type, const Transform& t = {}, Color3 c = k_scene_object_default_color)
    {
        const auto id = allocate_entity_id();
        if (id_to_index_.contains(id))
        {
            std::println(stderr, "Duplicate EntityId {}", id);
            std::terminate();
        }

        entities_.push_back(Entity{.id = id, .type = type, .transform = t, .color = c});
        Entity& out = entities_.back();
        id_to_index_.insert_or_assign(id, entities_.size() - 1zu);
        return out;
    }

    void remove_entity(EntityId id) noexcept
    {
        auto it = id_to_index_.find(id);
        if (it == id_to_index_.end())
        {
            return;
        }

        const auto idx = it->second;
        const auto last = entities_.empty() ? 0zu : (entities_.size() - 1zu);

        editor_state_.erase_from_selection(id);
        id_to_index_.erase(it);

        if (idx != last)
        {
            entities_[idx] = std::move(entities_[last]);
            id_to_index_.insert_or_assign(entities_[idx].id, idx);
        }
        entities_.pop_back();
    }

    [[nodiscard]] Entity* find(EntityId id) noexcept
    {
        auto it = id_to_index_.find(id);
        if (it == id_to_index_.end())
        {
            return nullptr;
        }
        return &entities_[it->second];
    }

    [[nodiscard]] const Entity* find(EntityId id) const noexcept
    {
        auto it = id_to_index_.find(id);
        if (it == id_to_index_.end())
        {
            return nullptr;
        }
        return &entities_[it->second];
    }

    [[nodiscard]] bool contains(EntityId id) const noexcept
    {
        return id_to_index_.contains(id);
    }

    [[nodiscard]] std::span<Entity> entities() noexcept
    {
        return std::span<Entity>{entities_.data(), entities_.size()};
    }

    [[nodiscard]] std::span<const Entity> entities() const noexcept
    {
        return std::span<const Entity>{entities_.data(), entities_.size()};
    }

    [[nodiscard]] Entity& entity(usize i) noexcept
    {
        return entities_[i];
    }

    [[nodiscard]] const Entity& entity(usize i) const noexcept
    {
        return entities_[i];
    }

    [[nodiscard]] Entity& entity_at(usize i)
    {
        return entities_.at(i);
    }

    [[nodiscard]] const Entity& entity_at(usize i) const
    {
        return entities_.at(i);
    }

    [[nodiscard]] EditorState& editor_state() noexcept
    {
        return editor_state_;
    }
    [[nodiscard]] const EditorState& editor_state() const noexcept
    {
        return editor_state_;
    }

  private:
    EntityId next_id_{0u};

    EditorState editor_state_{};
    EngineContext* engine_{};

    std::vector<Entity> entities_{};
    std::unordered_map<EntityId, usize> id_to_index_{};

    [[nodiscard]] EntityId allocate_entity_id() noexcept
    {
        const EntityId id = next_id_++;
        if (id == k_invalid_id)
        {
            std::println(stderr, "Generated invalid EntityId");
            std::terminate();
        }
        return id;
    }
};
}  // namespace ds_pba
