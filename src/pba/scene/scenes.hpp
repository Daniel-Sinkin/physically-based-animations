// pba/scene/scenes.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/scene/scene_types.hpp"

namespace ds_pba
{
struct EngineContext;

[[nodiscard]] constexpr usize scene_count() noexcept
{
    return static_cast<usize>(SceneId::Count);
}

[[nodiscard]] const char* scene_name(SceneId id) noexcept;
[[nodiscard]] const char* scene_description(SceneId id) noexcept;

// Loads a scene immediately (clears world + rebuilds), resets timing,
// and optionally pauses the simulation.
void load_scene(EngineContext& e, SceneId id, bool pause = true);

// Convenience wrappers (keeps your existing integration unchanged)
void setup_active_scene(EngineContext& e);
void update_active_scene(EngineContext& e, f32 frame_dt_s);

}  // namespace ds_pba
