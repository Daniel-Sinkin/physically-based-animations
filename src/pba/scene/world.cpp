#include "pba/scene/world.hpp"

#include <algorithm>
#include <print>

namespace ds_pba
{

auto EditorState::camera() noexcept -> Camera&
{
    return camera_;
}

auto EditorState::camera() const noexcept -> const Camera&
{
    return camera_;
}

auto EditorState::clear() noexcept -> void
{
    clear_selection();
    camera_.pivot = k_camera_pivot;
}

auto EditorState::has_selection() const noexcept -> bool
{
    return !selected_ids.empty();
}

auto EditorState::is_selected(EntityId id) const noexcept -> bool
{
    return std::find(selected_ids.begin(), selected_ids.end(), id) != selected_ids.end();
}

auto EditorState::clear_selection() noexcept -> void
{
    selected_ids.clear();
    active_id.reset();
}

auto EditorState::select_single(EntityId id) noexcept -> void
{
    selected_ids.clear();
    selected_ids.push_back(id);
    active_id = id;
}

auto EditorState::toggle_selection(EntityId id) noexcept -> void
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

auto EditorState::erase_from_selection(EntityId id) noexcept -> void
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

auto World::clear(bool reset_ids) noexcept -> void
{
    editor_state_.clear();

    entities_.clear();
    id_to_index_.clear();

    if (reset_ids)
    {
        next_id_ = 0u;
    }
}

auto World::spawn(EntityType type, const Transform& t, Color3 c) -> Entity&
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

auto World::remove_entity(EntityId id) noexcept -> void
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

auto World::find(EntityId id) noexcept -> Entity*
{
    auto it = id_to_index_.find(id);
    return (it == id_to_index_.end()) ? nullptr : &entities_[it->second];
}

auto World::find(EntityId id) const noexcept -> const Entity*
{
    auto it = id_to_index_.find(id);
    return (it == id_to_index_.end()) ? nullptr : &entities_[it->second];
}

auto World::contains(EntityId id) const noexcept -> bool
{
    return id_to_index_.contains(id);
}

auto World::entities() noexcept -> std::span<Entity>
{
    return {entities_.data(), entities_.size()};
}

auto World::entities() const noexcept -> std::span<const Entity>
{
    return {entities_.data(), entities_.size()};
}

auto World::entity(usize i) noexcept -> Entity&
{
    return entities_[i];
}

auto World::entity(usize i) const noexcept -> const Entity&
{
    return entities_[i];
}

auto World::entity_at(usize i) -> Entity&
{
    return entities_.at(i);
}

auto World::entity_at(usize i) const -> const Entity&
{
    return entities_.at(i);
}

auto World::editor_state() noexcept -> EditorState&
{
    return editor_state_;
}

auto World::editor_state() const noexcept -> const EditorState&
{
    return editor_state_;
}

auto World::allocate_entity_id() noexcept -> EntityId
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
