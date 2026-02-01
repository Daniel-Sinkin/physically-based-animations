#pragma once

#include "pba/core/constants.hpp"
#include "pba/scene/camera.hpp"
#include "pba/scene/entity.hpp"

#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace ds_pba
{

struct EngineContext;

class EditorState
{
  public:
    [[nodiscard]] Camera& camera() noexcept;
    [[nodiscard]] const Camera& camera() const noexcept;

    void clear() noexcept;

    [[nodiscard]] bool has_selection() const noexcept;
    [[nodiscard]] bool is_selected(EntityId id) const noexcept;

    void clear_selection() noexcept;
    void select_single(EntityId id) noexcept;
    void toggle_selection(EntityId id) noexcept;
    void erase_from_selection(EntityId id) noexcept;

    std::vector<EntityId> selected_ids{};
    std::optional<EntityId> active_id{};

  private:
    Camera camera_{};
};

class World
{
  public:
    World() = default;

    void clear(bool reset_ids = true) noexcept;

    Entity&
    spawn(EntityType type, const Transform& t = {}, Color3 c = k_scene_object_default_color);

    void remove_entity(EntityId id) noexcept;

    [[nodiscard]] Entity* find(EntityId id) noexcept;
    [[nodiscard]] const Entity* find(EntityId id) const noexcept;

    [[nodiscard]] bool contains(EntityId id) const noexcept;

    [[nodiscard]] std::span<Entity> entities() noexcept;
    [[nodiscard]] std::span<const Entity> entities() const noexcept;

    [[nodiscard]] Entity& entity(usize i) noexcept;
    [[nodiscard]] const Entity& entity(usize i) const noexcept;

    [[nodiscard]] Entity& entity_at(usize i);
    [[nodiscard]] const Entity& entity_at(usize i) const;

    [[nodiscard]] EditorState& editor_state() noexcept;
    [[nodiscard]] const EditorState& editor_state() const noexcept;

  private:
    [[nodiscard]] EntityId allocate_entity_id() noexcept;

    EntityId next_id_{0u};

    EditorState editor_state_{};

    std::vector<Entity> entities_{};
    std::unordered_map<EntityId, usize> id_to_index_{};
};

}  // namespace ds_pba
