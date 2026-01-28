// pba/engine/engine_context.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/engine/scene_id.hpp"
#include "pba/gfx/gfx_context.hpp"
#include "pba/physics/physics_context.hpp"
#include "pba/scene/entity.hpp"
#include "pba/scene/world.hpp"
//
#include <string>
#include <unordered_map>

namespace ds_pba
{
struct EngineContext
{
    GfxContext gfx{};
    PhysicsContext physics{};

    World world{};

    struct EntityLink
    {
        usize cube_obj_idx;
        usize physics_obj_idx;
    };

    TimePoint frame_time = Clock::now();
    Duration accumulator{};

    bool paused{true};

    void link_latest_objects(EntityId id);

    SceneId active_scene{k_default_scene};

    Entity& spawn_cube(
        Pos3 pos,
        Dir3 half_extents,
        f32 inv_mass,
        Dir3 vel,
        Quaternion ori,
        Dir3 ang_vel,
        Color3 color,
        std::string_view name
    );
    Entity& add_ground();

    void create_pyramid(int base_n, f32 step_size = 1.06f, f32 base_z = 0.5f);
    void create_pyramid_3d(int base_n, f32 step_size = 1.06f, f32 base_z = 0.5f);
    [[nodiscard]] bool setup();
    void run();
};

}  // namespace ds_pba
