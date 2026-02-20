// pba/simulation/scenes.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/simulation/scene_id.hpp"
//
#include <optional>
#include <span>
#include <string_view>

namespace ds_pba
{
struct SimulationContext;
struct SceneMetadata
{
    SceneId id{k_default_scene};
    std::string_view name{};
    std::string_view description{};
};

[[nodiscard]] auto scene_count() noexcept -> usize;
[[nodiscard]] auto scene_index(SceneId id) noexcept -> std::optional<usize>;
[[nodiscard]] auto scene_id_from_index(usize index) noexcept -> std::optional<SceneId>;
[[nodiscard]] auto scene_metadata(SceneId id) noexcept -> std::optional<SceneMetadata>;
[[nodiscard]] auto scene_catalog() noexcept -> std::span<const SceneMetadata>;

[[nodiscard]] auto scene_name(SceneId id) noexcept -> std::string_view;
[[nodiscard]] auto scene_description(SceneId id) noexcept -> std::string_view;

auto setup_scene_by_id(SimulationContext& e, SceneId id) noexcept -> void;

auto load_scene(SimulationContext& e, SceneId id) -> void;

auto setup_active_scene(SimulationContext& e) -> void;

}  // namespace ds_pba
