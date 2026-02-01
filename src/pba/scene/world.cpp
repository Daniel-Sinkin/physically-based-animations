#include "pba/scene/world.hpp"

#include <algorithm>
#include <print>

namespace ds_pba
{

Camera& EditorState::camera() noexcept
{
    return camera_;
}

const Camera& EditorState::camera() const noexcept
{
    return camera_;
}

void EditorState::clear() noexcept
{
    clear_selection();
    camera_.pivot = k_camera_pivot;
}

bool EditorState::has_selection() const noexcept
{
    return !selected_ids.empty();
}

bool EditorState::is_selected(EntityId id) const noexcept
{
    return std::find(selected_ids.begin(), selected_ids.end(), id) != selected_ids.end();
}

void EditorState::clear_selection() noexcept
{
    selected_ids.clear();
    active_id.reset();
}

void EditorState::select_single(EntityId id) noexcept
{
    selected_ids.clear();
    selected_ids.push_back(id);
    active_id = id;
}

void EditorState::toggle_selection(EntityId id) noexcept
{
    auto it = std::find(selected_ids.begin(), selected_ids.end(), id);
    if (it != selected_ids.end())
    {
        selected_ids.erase(it);

        if (active_id && *active_id == id)
        {
            active_id = selected_ids.empty() ? std::optional<EntityId>{}
                                             : std::optional<EntityId>{selected_ids.back()};
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

void EditorState::erase_from_selection(EntityId id) noexcept
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

void World::clear(bool reset_ids) noexcept
{
    editor_state_.clear();

    entities_.clear();
    id_to_index_.clear();

    if (reset_ids)
    {
        next_id_ = 0u;
    }
}

Entity& World::spawn(EntityType type, const Transform& t, Color3 c)
{
    const EntityId id = allocate_entity_id();

    if (id_to_index_.contains(id))
    {
        std::println(stderr, "Duplicate EntityId {}", id);
        std::terminate();
    }

    entities_.push_back(
        Entity{
            .id = id,
            .type = type,
            .transform = t,
            .color = c,
        }
    );

    id_to_index_.insert_or_assign(id, entities_.size() - 1zu);
    return entities_.back();
}

void World::remove_entity(EntityId id) noexcept
{
    auto it = id_to_index_.find(id);
    if (it == id_to_index_.end())
    {
        return;
    }

    const usize idx = it->second;
    const usize last = entities_.empty() ? 0zu : (entities_.size() - 1zu);

    editor_state_.erase_from_selection(id);
    id_to_index_.erase(it);

    if (idx != last)
    {
        entities_[idx] = std::move(entities_[last]);
        id_to_index_.insert_or_assign(entities_[idx].id, idx);
    }

    entities_.pop_back();
}

Entity* World::find(EntityId id) noexcept
{
    auto it = id_to_index_.find(id);
    return (it == id_to_index_.end()) ? nullptr : &entities_[it->second];
}

const Entity* World::find(EntityId id) const noexcept
{
    auto it = id_to_index_.find(id);
    return (it == id_to_index_.end()) ? nullptr : &entities_[it->second];
}

bool World::contains(EntityId id) const noexcept
{
    return id_to_index_.contains(id);
}

std::span<Entity> World::entities() noexcept
{
    return {entities_.data(), entities_.size()};
}

std::span<const Entity> World::entities() const noexcept
{
    return {entities_.data(), entities_.size()};
}

Entity& World::entity(usize i) noexcept
{
    return entities_[i];
}

const Entity& World::entity(usize i) const noexcept
{
    return entities_[i];
}

Entity& World::entity_at(usize i)
{
    return entities_.at(i);
}

const Entity& World::entity_at(usize i) const
{
    return entities_.at(i);
}

EditorState& World::editor_state() noexcept
{
    return editor_state_;
}

const EditorState& World::editor_state() const noexcept
{
    return editor_state_;
}

EntityId World::allocate_entity_id() noexcept
{
    const EntityId id = next_id_++;
    if (id == k_invalid_id)
    {
        std::println(stderr, "Generated invalid EntityId");
        std::terminate();
    }
    return id;
}

}  // namespace ds_pba
