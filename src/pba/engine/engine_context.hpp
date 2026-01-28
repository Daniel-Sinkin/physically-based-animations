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
    std::unordered_map<EntityId, EntityLink> obj_map{};
    std::unordered_map<EntityId, std::string> obj_name_map{};

    TimePoint frame_time = Clock::now();
    Duration accumulator{};

    bool paused{true};

    void link_latest_objects(EntityId id);

    SceneId active_scene{k_default_scene};

    void add_cube(Pos3 position);
    void spawn_cube(
        Pos3 pos,
        Dir3 vel = Dir3{0.0f, 0.0f, 0.0f},
        Quaternion ori = Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
        Color3 color = Color3{0.80f, 0.80f, 0.80f}
    );

    void add_ground();

    void create_pyramid(int base_n, f32 step_size = 1.06f, f32 base_z = 0.5f);
    void create_pyramid_3d(int base_n, f32 step_size = 1.06f, f32 base_z = 0.5f);
    [[nodiscard]] bool setup();
    void run();
};

}  // namespace ds_pba
