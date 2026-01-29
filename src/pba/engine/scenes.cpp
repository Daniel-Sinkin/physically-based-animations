// pba/engine/scenes.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine/scenes.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/math_types.hpp"
#include "pba/engine/engine_context.hpp"
//
#include <cmath>
//
#include <glm/ext/quaternion_trigonometric.hpp>

namespace ds_pba
{
// Defined in scenes_table.cpp (not public API)
void setup_scene_by_id(EngineContext& e, SceneId id) noexcept;

namespace
{
void reset_engine_world(EngineContext& e) noexcept
{
    e.world.clear();

    e.physics.bodies.clear();
    e.physics.external_forces.clear();
    e.physics.contact_cache.clear();

    e.physics.debug_contacts.clear();
    e.physics.debug_total_kinetic_energy = 0.0f;
    e.physics.debug_energy_sample_accum = Duration{0.0};
    e.physics.debug_total_kinetic_energy_history.clear();
}
}  // namespace

void load_scene(EngineContext& e, SceneId id, bool pause)
{
    e.active_scene = id;

    if (pause)
    {
        e.paused = true;
    }

    reset_engine_world(e);
    setup_scene_by_id(e, id);

    e.accumulator = Duration{0.0};
    e.frame_time = Clock::now();
    e.physics.time = e.frame_time;
}

void setup_active_scene(EngineContext& e)
{
    load_scene(e, e.active_scene, true);
}

}  // namespace ds_pba
