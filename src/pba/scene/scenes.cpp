// pba/scene/scenes.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/scene/scenes.hpp"
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
    e.scene.cube_objects.clear();
    e.scene.sphere_objects.clear();
    e.scene.hitmarker_objects.clear();
    e.scene.marble_bust_objects.clear();
    e.scene.clear_selection();
    e.scene.camera.pivot = k_camera_pivot;

    e.physics.bodies.clear();
    e.physics.external_forces.clear();
    e.physics.contact_cache.clear();

    e.physics.debug_contacts.clear();
    e.physics.debug_total_kinetic_energy = 0.0f;
    e.physics.debug_energy_sample_accum = Duration{0.0};
    e.physics.debug_total_kinetic_energy_history.clear();

    e.obj_map.clear();
    e.obj_name_map.clear();
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

void update_active_scene(EngineContext& e, f32 frame_dt_s)
{
    if (e.active_scene == SceneId::AttractorsAndRepulsivePivot)
    {
        if (!e.scene.marble_bust_objects.empty())
        {
            static f32 marble_spin_angle{k_pi};
            marble_spin_angle += 1.2f * frame_dt_s;
            if (marble_spin_angle > k_two_pi)
            {
                marble_spin_angle = std::fmod(marble_spin_angle, k_two_pi);
            }
            e.scene.marble_bust_objects[0].transform.orientation =
                glm::angleAxis(marble_spin_angle, k_axis_z);
        }
    }
}

}  // namespace ds_pba
