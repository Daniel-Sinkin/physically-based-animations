#pragma once

#include "pba/core/constants.hpp"
#include "pba/scene/camera.hpp"
#include "pba/scene/entity.hpp"
#include "pba/scene/entity_id.hpp"
#include "pba/scene/world_types.hpp"

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
    [[nodiscard]] auto camera() noexcept -> Camera&;
    [[nodiscard]] auto camera() const noexcept -> const Camera&;

    auto clear() noexcept -> void;

    [[nodiscard]] auto has_selection() const noexcept -> bool;
    [[nodiscard]] auto is_selected(EntityId id) const noexcept -> bool;

    auto clear_selection() noexcept -> void;
    auto select_single(EntityId id) noexcept -> void;
    auto toggle_selection(EntityId id) noexcept -> void;
    auto erase_from_selection(EntityId id) noexcept -> void;

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

    auto spawn(EntityType type, const Transform& t = {}, Color3 c = k_scene_object_default_color)
        -> Entity&;

    auto remove_entity(EntityId id) noexcept -> void;

    [[nodiscard]] auto find(EntityId id) noexcept -> Entity*;
    [[nodiscard]] auto find(EntityId id) const noexcept -> const Entity*;
    [[nodiscard]] auto find_idx(EntityId id) const noexcept -> std::optional<usize>;
    [[nodiscard]] auto contains(EntityId id) const noexcept -> bool;
    [[nodiscard]] auto entities() noexcept -> std::span<Entity>;
    [[nodiscard]] auto entities() const noexcept -> std::span<const Entity>;
    [[nodiscard]] auto entity(usize i) noexcept -> Entity&;
    [[nodiscard]] auto entity(usize i) const noexcept -> const Entity&;
    [[nodiscard]] auto entity_at(usize i) -> Entity&;
    [[nodiscard]] auto entity_at(usize i) const -> const Entity&;
    [[nodiscard]] auto transform_at(usize i) const noexcept -> std::optional<Transform>;
    [[nodiscard]] auto model_matrix_at(usize i) const noexcept -> std::optional<ModelMatrix>;
    auto set_transform(EntityId id, const Transform& t) noexcept -> bool;
    auto set_transform_at(usize i, const Transform& t) noexcept -> bool;
    auto set_position(EntityId id, const Pos3& p) noexcept -> bool;
    auto set_orientation(EntityId id, const Quaternion& q) noexcept -> bool;
    auto set_scale(EntityId id, const Dir3& s) noexcept -> bool;
    [[nodiscard]] auto editor_state() noexcept -> EditorState&;
    [[nodiscard]] auto editor_state() const noexcept -> const EditorState&;

  private:
    [[nodiscard]] EntityId allocate_entity_id() noexcept;

    EntityId next_id_{0u};

    EditorState editor_state_{};

    std::vector<Entity> entities_{};
    TransformSOA transforms_{k_max_number_objects};

    std::unordered_map<EntityId, usize> id_to_index_{};
};

}  // namespace ds_pba
