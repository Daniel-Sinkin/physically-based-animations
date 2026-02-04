// pba/simulation/simulation_context.hpp
#pragma once

#include "pba/physics/physics_context.hpp"
#include "pba/scene/world.hpp"
#include "pba/simulation/scene_id.hpp"
#include "pba/simulation/scenes.hpp"

namespace ds_pba
{
struct SimulationContext
{
    World world{};
    PhysicsContext physics{};
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
    auto create_box_body(const Entity& e, f32 inv_mass, Dir3 velo) const -> RigidBody;

    auto clear() noexcept -> void
    {
        world.clear();
        physics.clear();
    }
};
}  // namespace ds_pba
