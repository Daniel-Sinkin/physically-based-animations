// pba/engine/scenes.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine/scenes.hpp"
//
#include "glm/ext/quaternion_trigonometric.hpp"
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/engine/engine_context.hpp"
#include "pba/engine/scene_types.hpp"
#include "pba/physics/forces.hpp"
#include "pba/ui/ui.hpp"

#include <array>
#include <cmath>
#include <string_view>

namespace ds_pba
{
namespace
{

enum class SceneId : u32
{
    AttractorsAndRepulsivePivot,
    SmallPyramid_Projectiles_NoGround_Gravity,
    AttractorToOrigin_NoGravity,
    AttractorToOrigin_WithGravity,
    LargePyramid15_Ground_Gravity,
    Pyramid3D_HeavyCubeDrop,
    Motors_Elongated_NoGravity,
    NBody_SunAnd3Planets,
    NBody_ThreeBodyEqual,
    MovingAttractor_TargetMovesInCircle,
    OscillatingUniformForce_WithInternalTime,
    InclinedPlane_SlidingCubes,
    BoxDrop_Container,
    ProjectileWall,
};

static constexpr SceneId k_active_scene = SceneId::AttractorsAndRepulsivePivot;

[[nodiscard]] glm::mat3 inv_inertia_body_box(f32 inv_mass, const Direction3& half_extent) noexcept
{
    if (inv_mass == k_static_mass)
    {
        return glm::mat3(0.0f);
    }

    const auto m = 1.0f / inv_mass;

    const auto x = 2.0f * half_extent.x;
    const auto y = 2.0f * half_extent.y;
    const auto z = 2.0f * half_extent.z;

    const auto Ixx = (m / 12.0f) * (y * y + z * z);
    const auto Iyy = (m / 12.0f) * (x * x + z * z);
    const auto Izz = (m / 12.0f) * (x * x + y * y);

    glm::mat3 invI{0.0f};
    invI[0][0] = (Ixx > 0.0f) ? (1.0f / Ixx) : 0.0f;
    invI[1][1] = (Iyy > 0.0f) ? (1.0f / Iyy) : 0.0f;
    invI[2][2] = (Izz > 0.0f) ? (1.0f / Izz) : 0.0f;
    return invI;
}

[[nodiscard]] glm::mat3
inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
{
    const glm::mat3 R{glm::mat3_cast(q)};
    return R * inv_inertia_body * glm::transpose(R);
}

void reset_engine(EngineContext& e) noexcept
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

ObjectId spawn_box(
    EngineContext& e,
    Position3 pos,
    Direction3 half_extents,
    f32 inv_mass,
    Direction3 vel = Direction3{0.0f, 0.0f, 0.0f},
    Quaternion ori = Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
    Direction3 ang_vel = Direction3{0.0f, 0.0f, 0.0f},
    ColorRGBf color = ColorRGBf{k_scene_object_default_color},
    std::string_view name = {}
) noexcept
{
    const ObjectId id{next_object_id()};

    e.scene.cube_objects.push_back(
        Object{
            .id = id,
            .type = ObjectType::Cube,
            .transform =
                {
                    .position = pos,
                    .scale = half_extents * 2.0f,
                    .orientation = ori,
                },
            .color = color,
        }
    );

    RigidBody rb{
        .id = id,

        .half_extents = half_extents,

        .position = pos,
        .velocity = vel,
        .force_accum = Direction3{},
        .inv_mass = inv_mass,

        .orientation = ori,
        .angular_velocity = ang_vel,
        .torque_accum = Direction3{},

        .inv_inertia_body = glm::mat3(0.0f),
        .inv_inertia_world = glm::mat3(0.0f),
    };

    if (inv_mass != k_static_mass)
    {
        rb.inv_inertia_body = inv_inertia_body_box(inv_mass, half_extents);
        rb.inv_inertia_world = inv_inertia_world_from_body(rb.orientation, rb.inv_inertia_body);
    }

    e.physics.bodies.push_back(rb);

    e.link_latest_objects(id);

    if (!name.empty())
    {
        e.obj_name_map.insert_or_assign(id, std::string{name});
    }
    return id;
}

void spawn_static_ground(
    EngineContext& e,
    Position3 center,
    Direction3 half_extents,
    ColorRGBf color = ColorRGBf{0.10f, 0.10f, 0.10f},
    std::string_view name = "Ground"
) noexcept
{
    spawn_box(
        e,
        center,
        half_extents,
        k_static_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        color,
        name
    );
}

void create_pyramid_3d(EngineContext& e, int base_n, f32 step_size, f32 base_z) noexcept
{
    for (int layer{0}; layer < base_n; ++layer)
    {
        const auto n = base_n - layer;
        const auto z = base_z + static_cast<f32>(layer) * step_size;

        const auto half_span = 0.5f * static_cast<f32>(n - 1) * step_size;

        for (int ix{0}; ix < n; ++ix)
        {
            for (int iy{0}; iy < n; ++iy)
            {
                const f32 x{static_cast<f32>(ix) * step_size - half_span};
                const f32 y{static_cast<f32>(iy) * step_size - half_span};

                e.spawn_cube(
                    Position3{x, y, z},
                    k_zero_dir,
                    k_quaternion_identity,
                    ColorRGBf{k_scene_object_default_color}
                );
            }
        }
    }
}

[[nodiscard]] Direction3 tangent_ccw_xy(const Direction3& r) noexcept
{
    const Direction3 t{-r.y, r.x, 0.0f};
    const auto t2 = glm::dot(t, t);
    if (t2 <= 1e-12f)
    {
        return Direction3{0.0f, 1.0f, 0.0f};
    }
    return t / std::sqrt(t2);
}

struct KeepAwakeForce
{
};

void apply_keep_awake(std::vector<RigidBody>& bodies, f32, void*) noexcept
{
    for (RigidBody& b : bodies)
    {
        if (b.is_static())
        {
            continue;
        }
        b.asleep = false;
        b.sleep_frames = 0;
    }
}

struct NBodyForceParams
{
    f32 G{1.0f};
    f32 softening{1e-3f};
};

void apply_nbody_gravity_fixed(std::vector<RigidBody>& bodies, f32, void* user) noexcept
{
    auto& p = *static_cast<NBodyForceParams*>(user);
    const auto n = bodies.size();

    for (usize i = 0; i < n; ++i)
    {
        RigidBody& a = bodies[i];
        if (a.is_static() || a.asleep)
        {
            continue;
        }

        const auto ma = (a.inv_mass > 0.0f) ? (1.0f / a.inv_mass) : 0.0f;

        for (usize j = 0; j < n; ++j)
        {
            if (i == j)
            {
                continue;
            }
            const RigidBody& b = bodies[j];

            if (b.is_static())
            {
                continue;
            }
            const auto mb = (b.inv_mass > 0.0f) ? (1.0f / b.inv_mass) : 0.0f;

            const Direction3 r{b.position - a.position};
            const auto r2 = glm::dot(r, r) + p.softening * p.softening;
            const auto inv_r = 1.0f / std::sqrt(r2);
            const auto inv_r3 = inv_r * inv_r * inv_r;

            // F = G * ma * mb * r / |r|^3
            a.force_accum += (p.G * ma * mb) * r * inv_r3;
        }
    }
}

struct MovingAttractor
{
    Position3 target{};
    f32 time_s{0.0f};
    f32 omega{0.6f};
    f32 radius{16.0f};
    f32 z{2.0f};

    AttractorForce attractor{};
};

void apply_moving_attractor(std::vector<RigidBody>& bodies, f32 dt_s, void* user) noexcept
{
    auto& m = *static_cast<MovingAttractor*>(user);

    m.time_s += dt_s;

    const f32 ang = m.omega * m.time_s;
    m.target = Position3{
        m.radius * std::cos(ang),
        m.radius * std::sin(ang),
        m.z,
    };

    apply_attractor_force(bodies, dt_s, &m.attractor);
}

struct OscillatingUniform
{
    f32 time_s{0.0f};
    f32 omega{1.6f};

    Direction3 base_accel{};
    f32 amp_xy{8.0f};
};

void apply_oscillating_uniform(std::vector<RigidBody>& bodies, f32 dt_s, void* user) noexcept
{
    auto& f = *static_cast<OscillatingUniform*>(user);
    f.time_s += dt_s;

    const auto ang = f.omega * f.time_s;
    const Direction3 wind{
        f.amp_xy * std::cos(ang),
        f.amp_xy * std::sin(ang),
        0.0f,
    };

    UniformForce uf{.accel = f.base_accel + wind};
    apply_uniform_force(bodies, dt_s, &uf);
}

void setup_scene_current(EngineContext& e) noexcept
{
    e.spawn_cube(
        Position3{-14.0f, 0.0f, 2.0f},
        Direction3{+28.0f, 0.0f, 0.0f},
        k_quaternion_identity,
        ColorRGBf{0.90f, 0.35f, 0.35f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Projectile Red");

    e.spawn_cube(
        Position3{+14.0f, 0.0f, 2.2f},
        Direction3{-28.0f, 0.0f, 0.0f},
        k_quaternion_identity,
        ColorRGBf{0.35f, 0.90f, 0.35f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Projectile Green");

    e.spawn_cube(
        Position3{0.0f, -14.0f, 2.0f},
        Direction3{0.0f, +28.0f, 0.0f},
        k_quaternion_identity,
        ColorRGBf{0.35f, 0.55f, 0.95f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Projectile Blue");

    e.spawn_cube(
        Position3{0.0f, +14.0f, 2.4f},
        Direction3{0.0f, -28.0f, -2.0f},
        k_quaternion_identity,
        ColorRGBf{0.95f, 0.85f, 0.35f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Projectile Yellow");

    e.create_pyramid(16);

    e.scene.marble_bust_objects.push_back(
        Object{
            .id = next_object_id(),
            .type = ObjectType::MarbleBust,
            .transform{
                .position{10.0f, -15.0f, -5.0f},
                .scale{14.0f, 14.0f, 14.0f},
                .orientation{glm::angleAxis(k_pi, k_axis_z)}
            }
        }
    );

    e.scene.cube_objects.push_back(
        Object{
            .id = next_object_id(),
            .type = ObjectType::Cube,
            .transform{.position{10.0f, -15.0f, -5.5f}, .scale{10.0f, 10.0f, 1.0f}}
        }
    );

    static Position3 origin{0.0f, 0.0f, 0.0f};
    static Position3 pos{-20.0f, -10.0f, 10.0f};

    static AttractorForce attractor_origin{
        .target = &origin,
        .accel_mag = 15.0f,
        .min_radius = 0.5f,
    };

    static AttractorForce attractor_pos{
        .target = &pos,
        .accel_mag = 15.0f,
        .min_radius = 0.5f,
    };

    static RepulsionForce repulse_pivot{
        .target = nullptr,
        .accel_max = 100.0f,
        .range = 4.0f,
        .min_radius = 0.5f,
    };
    repulse_pivot.target = &e.scene.camera.pivot;

    e.physics.external_forces.push_back(
        ExternalForce{.fn = apply_attractor_force, .user = &attractor_origin}
    );
    e.physics.external_forces.push_back(
        ExternalForce{.fn = apply_repulsion_force, .user = &repulse_pivot}
    );
    e.physics.external_forces.push_back(
        ExternalForce{.fn = apply_attractor_force, .user = &attractor_pos}
    );
}

void setup_scene_small_pyramid_projectiles_gravity(EngineContext& e) noexcept
{
    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    e.spawn_cube(
        Position3{-12.0f, 0.0f, 10.0f},
        Direction3{+24.0f, 0.0f, 0.0f},
        k_quaternion_identity,
        ColorRGBf{0.90f, 0.35f, 0.35f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Projectile Red");

    e.spawn_cube(
        Position3{+12.0f, 0.0f, 10.2f},
        Direction3{-24.0f, 0.0f, 0.0f},
        k_quaternion_identity,
        ColorRGBf{0.35f, 0.90f, 0.35f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Projectile Green");

    e.spawn_cube(
        Position3{0.0f, -12.0f, 10.0f},
        Direction3{0.0f, +24.0f, 0.0f},
        k_quaternion_identity,
        ColorRGBf{0.35f, 0.55f, 0.95f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Projectile Blue");

    e.spawn_cube(
        Position3{0.0f, +12.0f, 10.4f},
        Direction3{0.0f, -24.0f, -2.0f},
        k_quaternion_identity,
        ColorRGBf{0.95f, 0.85f, 0.35f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Projectile Yellow");

    e.create_pyramid(8, 1.06f, 7.0f);
}

void setup_scene_attractor_origin_no_gravity(EngineContext& e) noexcept
{
    static Position3 origin{0.0f, 0.0f, 0.0f};
    static AttractorForce a{
        .target = &origin,
        .accel_mag = 14.0f,
        .min_radius = 0.9f,
    };
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_attractor_force, .user = &a});

    constexpr auto n = 30;
    const auto r = 16.0f;
    const auto vmag = 7.0f;

    for (int i = 0; i < n; ++i)
    {
        const f32 t = static_cast<f32>(i) / static_cast<f32>(n);
        const f32 ang = t * k_two_pi;

        const Position3 p{r * std::cos(ang), r * std::sin(ang), 2.0f};
        const Direction3 v{vmag * (-std::sin(ang)), vmag * (std::cos(ang)), 0.0f};

        e.spawn_cube(p, v, k_quaternion_identity, ColorRGBf{0.82f, 0.82f, 0.86f});
    }
}

void setup_scene_attractor_origin_with_gravity(EngineContext& e) noexcept
{
    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    static Position3 origin{0.0f, 0.0f, 0.0f};
    static AttractorForce a{
        .target = &origin,
        .accel_mag = 10.0f,
        .min_radius = 1.0f,
    };
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_attractor_force, .user = &a});

    constexpr int n = 26;
    const f32 r = 14.0f;
    const f32 vmag = 6.0f;

    for (int i = 0; i < n; ++i)
    {
        const auto i_f = static_cast<f32>(i);
        const f32 t = i_f / static_cast<f32>(n);
        const f32 ang = t * k_two_pi;

        const Position3 p{r * std::cos(ang), r * std::sin(ang), 12.0f + 0.15f * i_f};
        const Direction3 v{vmag * (-std::sin(ang)), vmag * (std::cos(ang)), 0.0f};

        e.spawn_cube(p, v, k_quaternion_identity, ColorRGBf{0.86f, 0.86f, 0.86f});
    }
}

void setup_scene_large_pyramid15_ground_gravity(EngineContext& e) noexcept
{
    spawn_static_ground(e, Position3{0.0f, 0.0f, -3.5f}, Direction3{40.0f, 40.0f, 0.5f});

    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    e.create_pyramid(15, 1.06f, -2.5f);
}

void setup_scene_pyramid3d_heavy_cube_drop(EngineContext& e) noexcept
{
    spawn_static_ground(e, Position3{0.0f, 0.0f, -3.5f}, Direction3{45.0f, 45.0f, 0.5f});

    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    create_pyramid_3d(e, 7, 1.06f, -2.5f);

    constexpr f32 heavy_mass = 350.0f;
    spawn_box(
        e,
        Position3{1.5f, 0.0f, 18.0f},
        Direction3{2.8f, 2.8f, 2.8f},
        1.0f / heavy_mass,
        Direction3{-1.0f, 0.0f, 0.0f},
        k_quaternion_identity,
        Direction3{0.0f, 0.0f, 0.4f},
        ColorRGBf{0.90f, 0.25f, 0.25f},
        "Heavy Cube"
    );
}

void setup_scene_motors_elongated_no_gravity(EngineContext& e) noexcept
{
    struct MotorPack
    {
        std::array<Motor, 8> motors{};
        usize count{0};
    };
    static MotorPack pack{};
    pack.count = 0;

    constexpr int k_rods = 6;
    for (int i = 0; i < k_rods; ++i)
    {
        const auto x = -12.0f + 4.8f * static_cast<f32>(i);
        const auto z = 2.0f + 0.25f * static_cast<f32>(i);

        const Quaternion q =
            (i % 2 == 0) ? k_quaternion_identity : glm::angleAxis(0.30f * k_pi, k_axis_z);

        const ObjectId id = spawn_box(
            e,
            Position3{x, 0.0f, z},
            Direction3{3.0f, 0.25f, 0.25f},
            1.0f / 8.0f,
            k_zero_dir,
            q,
            k_zero_dir,
            ColorRGBf{0.80f, 0.82f, 0.88f},
            std::string_view{}
        );

        e.obj_name_map.insert_or_assign(id, std::format("Motor Rod {}", i));

        pack.motors[pack.count] = Motor{
            .id = id,
            .torque = Direction3{0.0f, 0.0f, (i % 2 == 0) ? 70.0f : -70.0f},
        };
        e.physics.external_forces.push_back(
            ExternalForce{.fn = apply_motor_torque, .user = &pack.motors[pack.count]}
        );
        ++pack.count;
    }

    for (int i = 0; i < 20; ++i)
    {
        const f32 y = -9.0f + 0.95f * static_cast<f32>(i);
        e.spawn_cube(
            Position3{6.0f, y, 2.0f},
            k_zero_dir,
            k_quaternion_identity,
            ColorRGBf{0.45f, 0.70f, 0.95f}
        );
    }
}

void setup_scene_nbody_sun_3_planets(EngineContext& e) noexcept
{
    static NBodyForceParams p{
        .G = 3.0f,
        .softening = 0.6f,
    };

    e.physics.external_forces.push_back(ExternalForce{.fn = apply_keep_awake, .user = nullptr});
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_nbody_gravity_fixed, .user = &p});

    constexpr auto sun_mass = 1200.0f;
    const ObjectId sun_id = spawn_box(
        e,
        Position3{0.0f, 0.0f, 0.0f},
        Direction3{2.2f, 2.2f, 2.2f},
        1.0f / sun_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        ColorRGBf{0.95f, 0.75f, 0.25f},
        "Sun"
    );

    const auto r1 = 20.0f;
    const auto r2 = 30.0f;
    const auto r3 = 40.0f;

    const auto v1 = std::sqrt(p.G * sun_mass / r1);
    const auto v2 = std::sqrt(p.G * sun_mass / r2);
    const auto v3 = std::sqrt(p.G * sun_mass / r3);

    e.spawn_cube(
        Position3{+r1, 0.0f, 0.0f},
        Direction3{0.0f, +v1, 0.0f},
        k_quaternion_identity,
        ColorRGBf{0.35f, 0.65f, 0.95f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Planet A");

    e.spawn_cube(
        Position3{0.0f, +r2, 0.0f},
        Direction3{-v2, 0.0f, 0.0f},
        k_quaternion_identity,
        ColorRGBf{0.35f, 0.95f, 0.55f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Planet B");

    e.spawn_cube(
        Position3{-r3, 0.0f, 0.0f},
        Direction3{0.0f, -v3, 0.0f},
        k_quaternion_identity,
        ColorRGBf{0.95f, 0.35f, 0.75f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Planet C");

    const Direction3 p_total = Direction3{0.0f, v1 - v3, 0.0f} + Direction3{-v2, 0.0f, 0.0f};
    const Direction3 v_sun = -(p_total / sun_mass);

    for (RigidBody& b : e.physics.bodies)
    {
        if (b.id == sun_id)
        {
            b.velocity = v_sun;
            break;
        }
    }
}

void setup_scene_nbody_three_body_equal(EngineContext& e) noexcept
{
    static NBodyForceParams p{
        .G = 40.0f,
        .softening = 0.5f,
    };

    e.physics.external_forces.push_back(ExternalForce{.fn = apply_keep_awake, .user = nullptr});
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_nbody_gravity_fixed, .user = &p});

    constexpr auto m = 60.0f;
    const auto inv_m = 1.0f / m;

    const auto R = 20.0f;
    const auto y = R * std::sqrt(3.0f) * 0.5f;

    const std::array<Position3, 3> pos = {
        Position3{+R, 0.0f, 0.0f},
        Position3{-0.5f * R, +y, 0.0f},
        Position3{-0.5f * R, -y, 0.0f},
    };

    // v = sqrt(G*m / (sqrt(3)*R))
    const auto v = std::sqrt(p.G * m / (std::sqrt(3.0f) * R));

    for (int i = 0; i < 3; ++i)
    {
        const Direction3 r{pos[static_cast<usize>(i)]};
        const Direction3 t{tangent_ccw_xy(r)};

        f32 vi = v;
        if (i == 2)
        {
            vi *= 1.03f;
        }

        spawn_box(
            e,
            pos[static_cast<usize>(i)],
            Direction3{0.8f, 0.8f, 0.8f},
            inv_m,
            vi * t,
            k_quaternion_identity,
            k_zero_dir,
            ColorRGBf{0.85f, 0.85f, 0.90f},
            std::format("Body {}", i)
        );
    }
}

void setup_scene_moving_attractor_circle(EngineContext& e) noexcept
{
    constexpr int n = 42;
    for (int i = 0; i < n; ++i)
    {
        const f32 t = static_cast<f32>(i) / static_cast<f32>(n);
        const f32 ang = t * k_two_pi;

        const f32 r = 10.0f + 0.25f * static_cast<f32>(i);
        const Position3 p{
            r * std::cos(ang),
            r * std::sin(ang),
            2.0f + 0.15f * std::sin(3.0f * ang),
        };

        const Direction3 v{
            -2.0f * std::sin(ang),
            +2.0f * std::cos(ang),
            0.0f,
        };

        e.spawn_cube(p, v, k_quaternion_identity, ColorRGBf{0.80f, 0.85f, 0.90f});
    }

    static MovingAttractor m{};
    m.time_s = 0.0f;
    m.omega = 0.7f;
    m.radius = 18.0f;
    m.z = 2.0f;

    m.attractor = AttractorForce{
        .target = &m.target,
        .accel_mag = 18.0f,
        .min_radius = 1.0f,
    };

    e.physics.external_forces.push_back(ExternalForce{.fn = apply_moving_attractor, .user = &m});
}

void setup_scene_oscillating_uniform_force(EngineContext& e) noexcept
{
    spawn_static_ground(e, Position3{0.0f, 0.0f, -3.5f}, Direction3{35.0f, 35.0f, 0.5f});

    for (int i = 0; i < 12; ++i)
    {
        e.spawn_cube(Position3{0.0f, 0.0f, -2.5f + 1.02f * static_cast<f32>(i)});
    }

    static OscillatingUniform f{};
    f.time_s = 0.0f;
    f.omega = 1.3f;
    f.base_accel = k_gravity;
    f.amp_xy = 10.0f;

    e.physics.external_forces.push_back(ExternalForce{.fn = apply_oscillating_uniform, .user = &f});
}

void setup_scene_inclined_plane(EngineContext& e) noexcept
{
    spawn_static_ground(e, Position3{0.0f, 0.0f, -4.0f}, Direction3{50.0f, 50.0f, 0.5f});

    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    const Quaternion ramp_q = glm::angleAxis(-0.22f * k_pi, k_axis_y);
    spawn_box(
        e,
        Position3{-6.0f, 0.0f, -1.5f},
        Direction3{18.0f, 6.0f, 0.6f},
        k_static_mass,
        k_zero_dir,
        ramp_q,
        k_zero_dir,
        ColorRGBf{0.22f, 0.22f, 0.22f},
        "Ramp"
    );

    for (int i = 0; i < 24; ++i)
    {
        const auto x = -18.0f + 1.2f * static_cast<f32>(i);
        const auto y = 0.0f;
        const auto z = 7.0f + 0.35f * static_cast<f32>(i);
        e.spawn_cube(Position3{x, y, z});
    }
}

void setup_scene_box_drop_container(EngineContext& e) noexcept
{
    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    spawn_static_ground(e, Position3{0.0f, 0.0f, -3.5f}, Direction3{22.0f, 22.0f, 0.5f});

    const auto wall_th = 0.6f;
    const auto wall_hz = 9.0f;
    const auto inner = 17.0f;
    const auto wall_cz = -3.0f + wall_hz;

    spawn_box(
        e,
        Position3{+(inner + wall_th), 0.0f, wall_cz},
        Direction3{wall_th, inner, wall_hz},
        k_static_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        ColorRGBf{0.12f, 0.12f, 0.12f},
        "+X Wall"
    );
    spawn_box(
        e,
        Position3{-(inner + wall_th), 0.0f, wall_cz},
        Direction3{wall_th, inner, wall_hz},
        k_static_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        ColorRGBf{0.12f, 0.12f, 0.12f},
        "-X Wall"
    );
    spawn_box(
        e,
        Position3{0.0f, +(inner + wall_th), wall_cz},
        Direction3{inner, wall_th, wall_hz},
        k_static_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        ColorRGBf{0.12f, 0.12f, 0.12f},
        "+Y Wall"
    );
    spawn_box(
        e,
        Position3{0.0f, -(inner + wall_th), wall_cz},
        Direction3{inner, wall_th, wall_hz},
        k_static_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        ColorRGBf{0.12f, 0.12f, 0.12f},
        "-Y Wall"
    );

    for (int i{0}; i < 80; ++i)
    {
        const auto fx = static_cast<f32>((i * 37) % 100) / 100.0f;
        const auto fy = static_cast<f32>((i * 73) % 100) / 100.0f;

        const auto x = (fx * 2.0f - 1.0f) * (inner - 2.5f);
        const auto y = (fy * 2.0f - 1.0f) * (inner - 2.5f);
        const auto z = 2.0f + 0.32f * static_cast<f32>(i);

        e.spawn_cube(Position3{x, y, z});
    }
}

void setup_scene_projectile_wall(EngineContext& e) noexcept
{
    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    spawn_static_ground(e, Position3{0.0f, 0.0f, -3.5f}, Direction3{45.0f, 45.0f, 0.5f});

    const auto nx = 14;
    const auto nz = 9;
    const auto step = 1.02f;
    const auto x0 = -0.5f * (nx - 1) * step;
    const auto z0 = -2.5f;

    for (int iz = 0; iz < nz; ++iz)
    {
        for (int ix = 0; ix < nx; ++ix)
        {
            const auto x = x0 + static_cast<f32>(ix) * step;
            const auto y = 0.0f;
            const auto z = z0 + static_cast<f32>(iz) * step;
            e.spawn_cube(Position3{x, y, z});
        }
    }

    e.spawn_cube(
        Position3{0.0f, -22.0f, 2.5f},
        Direction3{0.0f, +36.0f, 0.0f},
        k_quaternion_identity,
        ColorRGBf{0.95f, 0.35f, 0.35f}
    );
    e.obj_name_map.insert_or_assign(e.scene.cube_objects.back().id, "Wall Projectile");
}

}  // namespace

void setup_active_scene(EngineContext& e)
{
    reset_engine(e);

    ui_log(std::format("Active scene: {}", static_cast<u32>(k_active_scene)));

    switch (k_active_scene)
    {
        case SceneId::AttractorsAndRepulsivePivot:
            setup_scene_current(e);
            break;

        case SceneId::SmallPyramid_Projectiles_NoGround_Gravity:
            setup_scene_small_pyramid_projectiles_gravity(e);
            break;

        case SceneId::AttractorToOrigin_NoGravity:
            setup_scene_attractor_origin_no_gravity(e);
            break;

        case SceneId::AttractorToOrigin_WithGravity:
            setup_scene_attractor_origin_with_gravity(e);
            break;

        case SceneId::LargePyramid15_Ground_Gravity:
            setup_scene_large_pyramid15_ground_gravity(e);
            break;

        case SceneId::Pyramid3D_HeavyCubeDrop:
            setup_scene_pyramid3d_heavy_cube_drop(e);
            break;

        case SceneId::Motors_Elongated_NoGravity:
            setup_scene_motors_elongated_no_gravity(e);
            break;

        case SceneId::NBody_SunAnd3Planets:
            setup_scene_nbody_sun_3_planets(e);
            break;

        case SceneId::NBody_ThreeBodyEqual:
            setup_scene_nbody_three_body_equal(e);
            break;

        case SceneId::MovingAttractor_TargetMovesInCircle:
            setup_scene_moving_attractor_circle(e);
            break;

        case SceneId::OscillatingUniformForce_WithInternalTime:
            setup_scene_oscillating_uniform_force(e);
            break;

        case SceneId::InclinedPlane_SlidingCubes:
            setup_scene_inclined_plane(e);
            break;

        case SceneId::BoxDrop_Container:
            setup_scene_box_drop_container(e);
            break;

        case SceneId::ProjectileWall:
            setup_scene_projectile_wall(e);
            break;
    }
}

void update_active_scene(EngineContext& e, f32 frame_dt_s)
{
    if constexpr (k_active_scene == SceneId::AttractorsAndRepulsivePivot)
    {
        if (!e.scene.marble_bust_objects.empty())
        {
            static auto marble_spin_angle = k_pi;
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
