// pba/simulation/scenes.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/simulation/scenes.hpp"
//
#include "pba/simulation/simulation_context.hpp"
//
#include <glm/ext/quaternion_trigonometric.hpp>

namespace ds_pba
{

auto load_scene(SimulationContext& e, SceneId id) -> void
{
    e.clear();
    const auto selected = scene_metadata(id).has_value() ? id : k_default_scene;
    e.active_scene = selected;
    setup_scene_by_id(e, selected);
}

auto setup_active_scene(SimulationContext& e) -> void
{
    load_scene(e, e.active_scene);
}
}  // namespace ds_pba
