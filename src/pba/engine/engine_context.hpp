// pba/engine/engine_context.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/engine/scene_id.hpp"
#include "pba/gfx/gfx_context.hpp"
#include "pba/physics/physics_context.hpp"
#include "pba/physics/physics_types.hpp"
#include "pba/scene/entity.hpp"
#include "pba/scene/world.hpp"

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

    auto spawn_cube(
        Pos3 pos,
        Dir3 half_extents,
        f32 inv_mass,
        Dir3 vel,
        Quaternion ori,
        Dir3 ang_vel,
        Color3 color,
        std::string_view name
    ) -> Entity&;
    auto add_ground() -> Entity&;

    auto create_pyramid(int base_n, f32 step_size = 1.06f, f32 base_z = 0.5f) -> void;
    auto create_pyramid_3d(int base_n, f32 step_size = 1.06f, f32 base_z = 0.5f) -> void;
    [[nodiscard]] auto setup() -> bool;
    auto sync_physics_to_world() -> void;
    auto run() -> void;

    auto create_box_body(const Entity& e, f32 inv_mass, Dir3 velo) const -> RigidBody;
};

}  // namespace ds_pba
