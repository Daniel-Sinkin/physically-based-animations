// pba/engine/scenes.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine/scenes.hpp"
//
#include "pba/engine/engine_context.hpp"
#include "pba/simulation/simulation_context.hpp"
//
#include <glm/ext/quaternion_trigonometric.hpp>

namespace ds_pba
{
auto setup_scene_by_id(SimulationContext& e, SceneId id) noexcept -> void;

auto load_scene(SimulationContext& sim, SceneId id) -> void
{
    sim.active_scene = id;

    sim.clear();
    setup_scene_by_id(sim, id);
}

auto setup_active_scene(SimulationContext& sim) -> void
{
    load_scene(sim, sim.active_scene);
}

}  // namespace ds_pba
