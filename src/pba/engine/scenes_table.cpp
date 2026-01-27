// pba/engine/scenes_table.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine/scenes.hpp"
//
#include <array>

namespace ds_pba
{
void setup_scene_attractors_and_repulsive_pivot(EngineContext& e) noexcept;
void setup_scene_small_pyramid_projectiles_gravity(EngineContext& e) noexcept;
void setup_scene_attractor_origin_no_gravity(EngineContext& e) noexcept;
void setup_scene_attractor_origin_with_gravity(EngineContext& e) noexcept;
void setup_scene_large_pyramid15_ground_gravity(EngineContext& e) noexcept;
void setup_scene_pyramid3d_heavy_cube_drop(EngineContext& e) noexcept;
void setup_scene_motors_elongated_no_gravity(EngineContext& e) noexcept;
void setup_scene_nbody_sun_3_planets(EngineContext& e) noexcept;
void setup_scene_nbody_three_body_equal(EngineContext& e) noexcept;
void setup_scene_moving_attractor_circle(EngineContext& e) noexcept;
void setup_scene_oscillating_uniform_force(EngineContext& e) noexcept;
void setup_scene_inclined_plane(EngineContext& e) noexcept;
void setup_scene_box_drop_container(EngineContext& e) noexcept;
void setup_scene_projectile_wall(EngineContext& e) noexcept;

namespace
{
using SetupSceneFn = void (*)(EngineContext&) noexcept;

struct Scene
{
    SceneId id{};
    const char* name{};
    const char* desc{};
    SetupSceneFn setup{};
};

[[nodiscard]] constexpr usize scene_index(SceneId id) noexcept
{
    return static_cast<usize>(id);
}

static constexpr std::array<Scene, scene_count()> k_scenes = {{
    {SceneId::AttractorsAndRepulsivePivot,
     "Current: 2 attractors + repulsive pivot",
     "Your current testbed: 4 projectiles + flat pyramid + marble bust; forces: attract to "
     "origin, attract to fixed point, repulsion from camera pivot; no gravity.",
     &setup_scene_attractors_and_repulsive_pivot},

    {SceneId::SmallPyramid_Projectiles_NoGround_Gravity,
     "Small pyramid + projectiles (gravity, no ground)",
     "Small flat pyramid + 4 projectiles, no ground; gravity enabled so everything collides "
     "mid-air while falling.",
     &setup_scene_small_pyramid_projectiles_gravity},

    {SceneId::AttractorToOrigin_NoGravity,
     "Attractor to origin (no gravity)",
     "Ring/swarm of cubes with tangential velocity; constant-magnitude attractor pulls toward "
     "origin; no gravity.",
     &setup_scene_attractor_origin_no_gravity},

    {SceneId::AttractorToOrigin_WithGravity,
     "Attractor to origin + gravity (no ground)",
     "Like attractor-to-origin but also gravity enabled; objects fall while being pulled "
     "inward; no ground.",
     &setup_scene_attractor_origin_with_gravity},

    {SceneId::LargePyramid15_Ground_Gravity,
     "Large pyramid (15) + large ground (gravity)",
     "Large flat pyramid (base 15) resting on large static ground; gravity enabled; "
     "bulk-contact stress test.",
     &setup_scene_large_pyramid15_ground_gravity},

    {SceneId::Pyramid3D_HeavyCubeDrop,
     "3D pyramid + heavy cube drop",
     "Smaller square (3D) pyramid on ground; gravity enabled; a large high-mass cube drops "
     "onto it from above.",
     &setup_scene_pyramid3d_heavy_cube_drop},

    {SceneId::Motors_Elongated_NoGravity,
     "Motors: elongated boxes (no gravity)",
     "No gravity; several elongated 'rods' with motor torque applied; tests angular "
     "integration/inertia + contacts.",
     &setup_scene_motors_elongated_no_gravity},

    {SceneId::NBody_SunAnd3Planets,
     "N-body: Sun + 3 planets",
     "N-body gravity (force-based) with one heavy 'sun' and 3 lighter planets with tangential "
     "initial velocities.",
     &setup_scene_nbody_sun_3_planets},

    {SceneId::NBody_ThreeBodyEqual,
     "N-body: 3 equal-mass bodies (3-body problem)",
     "Three equal-mass bodies on an equilateral triangle; start near rotating solution, "
     "perturb one -> chaos.",
     &setup_scene_nbody_three_body_equal},

    {SceneId::MovingAttractor_TargetMovesInCircle,
     "Moving attractor (target moves in a circle)",
     "Time-accumulator: attractor target position moves on a large circle; swarm gets dragged "
     "around.",
     &setup_scene_moving_attractor_circle},

    {SceneId::OscillatingUniformForce_WithInternalTime,
     "Oscillating uniform force (internal time)",
     "Internal time: gravity + rotating horizontal 'wind' (uniform accel) applied; stack "
     "reacts to changing force.",
     &setup_scene_oscillating_uniform_force},

    {SceneId::InclinedPlane_SlidingCubes,
     "Inclined plane (gravity)",
     "Static tilted ramp + gravity; cubes slide/bounce; friction + OBB contact stability test.",
     &setup_scene_inclined_plane},

    {SceneId::BoxDrop_Container,
     "Container drop (gravity)",
     "Ground + 4 walls container; many cubes dropped in; dense stacking stress test (still "
     "O(N^2) contacts).",
     &setup_scene_box_drop_container},

    {SceneId::ProjectileWall,
     "Projectile vs wall (gravity)",
     "Wall of cubes on ground with gravity; one fast projectile impacts the wall.",
     &setup_scene_projectile_wall},
}};

static_assert(
    k_scenes.size() == static_cast<usize>(SceneId::Count),
    "k_scenes must have one Scene per SceneId (excluding Count)."
);

constexpr bool scenes_are_consistent() noexcept
{
    for (usize i{0zu}; i < k_scenes.size(); ++i)
    {
        if (scene_index(k_scenes[i].id) != i)
        {
            return false;
        }
        if (k_scenes[i].setup == nullptr)
        {
            return false;
        }
    }
    return true;
}
static_assert(scenes_are_consistent(), "Scene table order or ids are inconsistent.");

[[nodiscard]] constexpr const Scene* scene_ptr(SceneId id) noexcept
{
    const usize idx = scene_index(id);
    if (idx >= k_scenes.size())
    {
        return nullptr;
    }
    return &k_scenes[idx];
}
}  // namespace

[[nodiscard]] const char* scene_name(SceneId id) noexcept
{
    const Scene* s = scene_ptr(id);
    return s ? s->name : "(invalid scene)";
}

[[nodiscard]] const char* scene_description(SceneId id) noexcept
{
    const Scene* s = scene_ptr(id);
    return s ? s->desc : "(invalid scene)";
}

void setup_scene_by_id(EngineContext& e, SceneId id) noexcept
{
    const Scene* s = scene_ptr(id);
    if (s && s->setup)
    {
        s->setup(e);
    }
}

}  // namespace ds_pba
