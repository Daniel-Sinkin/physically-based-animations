#include "pba/scene/world.hpp"

#include "pba/scene/entity_id.hpp"

#include <algorithm>
#include <optional>
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
    transforms_.reset();
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

    const auto transform_idx = transforms_.push_back(t);
    if (!transform_idx)
    {
        std::println(
            stderr,
            "TransformSOA capacity exceeded (capacity={}, id={})",
            transforms_.capacity(),
            id
        );
        std::terminate();
    }

    if (*transform_idx != entities_.size())
    {
        std::println(
            stderr,
            "Transform index mismatch (transform_idx={}, entity_idx={})",
            *transform_idx,
            entities_.size()
        );
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

    const auto idx = it->second;
    const auto last = entities_.empty() ? 0zu : (entities_.size() - 1zu);

    editor_state_.erase_from_selection(id);
    id_to_index_.erase(it);

    const auto transform_ok = transforms_.swap_erase(idx);
    if (!transform_ok)
    {
        std::println(stderr, "TransformSOA erase failed for entity {}", id);
        std::terminate();
    }

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

auto World::find_idx(EntityId id) const noexcept -> std::optional<usize>
{
    if (auto it = id_to_index_.find(id); it != id_to_index_.end())
    {
        return it->second;
    }
    return std::nullopt;
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

auto World::transform_at(usize i) const noexcept -> std::optional<Transform>
{
    return transforms_.get(i);
}

auto World::model_matrix_at(usize i) const noexcept -> std::optional<ModelMatrix>
{
    if (const auto t = transforms_.get(i))
    {
        return t->model_matrix();
    }
    return std::nullopt;
}

auto World::set_transform(EntityId id, const Transform& t) noexcept -> bool
{
    const auto idx = find_idx(id);
    if (!idx)
    {
        return false;
    }
    return set_transform_at(*idx, t);
}

auto World::set_transform_at(usize i, const Transform& t) noexcept -> bool
{
    if (i >= entities_.size())
    {
        return false;
    }
    if (!transforms_.set(i, t))
    {
        return false;
    }
    entities_[i].transform = t;
    return true;
}

auto World::set_position(EntityId id, const Pos3& p) noexcept -> bool
{
    const auto idx = find_idx(id);
    if (!idx)
    {
        return false;
    }
    if (!transforms_.set_position(*idx, p))
    {
        return false;
    }
    entities_[*idx].transform.position = p;
    return true;
}

auto World::set_orientation(EntityId id, const Quaternion& q) noexcept -> bool
{
    const auto idx = find_idx(id);
    if (!idx)
    {
        return false;
    }
    if (!transforms_.set_orientation(*idx, q))
    {
        return false;
    }
    entities_[*idx].transform.orientation = q;
    return true;
}

auto World::set_scale(EntityId id, const Dir3& s) noexcept -> bool
{
    const auto idx = find_idx(id);
    if (!idx)
    {
        return false;
    }
    if (!transforms_.set_scale(*idx, s))
    {
        return false;
    }
    entities_[*idx].transform.scale = s;
    return true;
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
