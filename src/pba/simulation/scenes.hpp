// pba/simulation/scenes.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/simulation/scene_id.hpp"

namespace ds_pba
{
struct SimulationContext;

[[nodiscard]] constexpr auto scene_count() noexcept -> usize
{
    return static_cast<usize>(SceneId::Count);
}

[[nodiscard]] auto scene_name(SceneId id) noexcept -> std::string;
[[nodiscard]] auto scene_description(SceneId id) noexcept -> std::string;

auto setup_scene_by_id(SimulationContext& e, SceneId id) noexcept -> void;

auto load_scene(SimulationContext& e, SceneId id, bool pause = true) -> void;

auto setup_active_scene(SimulationContext& e) -> void;

}  // namespace ds_pba
