// pba/engine/scenes.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/engine/scene_id.hpp"

namespace ds_pba
{
struct EngineContext;

[[nodiscard]] constexpr auto scene_count() noexcept -> usize
{
    return static_cast<usize>(SceneId::Count);
}

[[nodiscard]] auto scene_name(SceneId id) noexcept -> const char*;
[[nodiscard]] auto scene_description(SceneId id) noexcept -> const char*;

auto load_scene(EngineContext& e, SceneId id, bool pause = true) -> void;

auto setup_active_scene(EngineContext& e) -> void;

}  // namespace ds_pba
