// pba/scene/scenes_setup.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/engine/engine_context.hpp"
#include "pba/physics/forces.hpp"
#include "pba/scene/entity.hpp"
#include "pba/scene/world_types.hpp"
//
#include <array>
#include <cmath>
#include <string_view>
//
#include <glm/ext/quaternion_trigonometric.hpp>

namespace ds_pba
{
namespace
{

[[nodiscard]] EntityId spawn_box(
    EngineContext& e,
    Pos3 pos,
    Dir3 half_extents,
    f32 inv_mass,
    Dir3 vel = k_zero_dir,
    Quaternion ori = k_quaternion_identity,
    Dir3 ang_vel = k_zero_dir,
    Color3 color = Color3{k_scene_object_default_color},
    std::string_view name = {}
) noexcept
{
    Entity& ent = e.spawn_cube(pos, half_extents, inv_mass, vel, ori, ang_vel, color, name);
    return ent.id;
}

EntityId spawn_cube(
    EngineContext& e,
    Pos3 pos,
    Dir3 vel = k_zero_dir,
    Quaternion ori = k_quaternion_identity,
    Color3 color = Color3{k_scene_object_default_color},
    std::string_view name = {}
) noexcept
{
    constexpr Dir3 k_half_extents{0.5f, 0.5f, 0.5f};
    constexpr f32 k_inv_mass{1.0f};
    return spawn_box(e, pos, k_half_extents, k_inv_mass, vel, ori, k_zero_dir, color, name);
}

void spawn_static_ground(
    EngineContext& e,
    Pos3 center,
    Dir3 half_extents,
    Color3 color = Color3{0.10f, 0.10f, 0.10f},
    std::string_view name = "Ground"
) noexcept
{
    (void) spawn_box(
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
        const int n = base_n - layer;
        const f32 z = base_z + static_cast<f32>(layer) * step_size;

        const f32 half_span = 0.5f * static_cast<f32>(n - 1) * step_size;

        for (int ix{0}; ix < n; ++ix)
        {
            const f32 x = static_cast<f32>(ix) * step_size - half_span;
            for (int iy{0}; iy < n; ++iy)
            {
                const f32 y = static_cast<f32>(iy) * step_size - half_span;
                spawn_cube(
                    e,
                    Pos3{x, y, z},
                    k_zero_dir,
                    k_quaternion_identity,
                    Color3{k_scene_object_default_color}
                );
            }
        }
    }
}

Dir3 tangent_ccw_xy(const Dir3& r) noexcept
{
    const Dir3 t{-r.y, r.x, 0.0f};
    const auto t2 = glm::dot(t, t);
    if (t2 <= 1e-12f)
    {
        return Dir3{0.0f, 1.0f, 0.0f};
    }
    return t / std::sqrt(t2);
}

void apply_keep_awake(std::span<RigidBody> bodies, f32, void*) noexcept
{
    for (auto& b : bodies)
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

void apply_nbody_gravity_fixed(std::span<RigidBody> bodies, f32, void* user) noexcept
{
    auto& p = *static_cast<NBodyForceParams*>(user);
    const usize n = bodies.size();
    for (usize i{0}; i < n; ++i)
    {
        RigidBody& a = bodies[i];
        if (a.is_static() || a.asleep)
        {
            continue;
        }

        const auto ma = (a.inv_mass > 0.0f) ? (1.0f / a.inv_mass) : 0.0f;

        for (usize j{0zu}; j < n; ++j)
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

            const Dir3 r{b.position - a.position};
            const auto r2 = glm::dot(r, r) + p.softening * p.softening;
            const auto inv_r = 1.0f / std::sqrt(r2);
            const auto inv_r3 = inv_r * inv_r * inv_r;

            a.force_accum += (p.G * ma * mb) * r * inv_r3;
        }
    }
}

struct MovingAttractor
{
    Pos3 target{};
    f32 time_s{0.0f};
    f32 omega{0.6f};
    f32 radius{16.0f};
    f32 z{2.0f};

    AttractorForce attractor{};
};

void apply_moving_attractor(std::span<RigidBody> bodies, f32 dt_s, void* user) noexcept
{
    auto& m = *static_cast<MovingAttractor*>(user);

    m.time_s += dt_s;
    const auto ang = m.omega * m.time_s;

    m.target = Pos3{
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

    Dir3 base_accel{};
    f32 amp_xy{8.0f};
};

void apply_oscillating_uniform(std::span<RigidBody> bodies, f32 dt_s, void* user) noexcept
{
    auto& f = *static_cast<OscillatingUniform*>(user);
    f.time_s += dt_s;

    const auto ang = f.omega * f.time_s;
    const Dir3 wind{
        f.amp_xy * std::cos(ang),
        f.amp_xy * std::sin(ang),
        0.0f,
    };

    UniformForce uf{.accel = f.base_accel + wind};
    apply_uniform_force(bodies, dt_s, &uf);
}

}  // namespace

void setup_scene_attractors_and_repulsive_pivot(EngineContext& e) noexcept
{
    spawn_cube(
        e,
        Pos3{-14.0f, 0.0f, 2.0f},
        Dir3{+28.0f, 0.0f, 0.0f},
        k_quaternion_identity,
        Color3{0.90f, 0.35f, 0.35f},
        "Projectile Red"
    );

    spawn_cube(
        e,
        Pos3{+14.0f, 0.0f, 2.2f},
        Dir3{-28.0f, 0.0f, 0.0f},
        k_quaternion_identity,
        Color3{0.35f, 0.90f, 0.35f},
        "Projectile Green"
    );

    spawn_cube(
        e,
        Pos3{0.0f, -14.0f, 2.0f},
        Dir3{0.0f, +28.0f, 0.0f},
        k_quaternion_identity,
        Color3{0.35f, 0.55f, 0.95f},
        "Projectile Blue"
    );

    spawn_cube(
        e,
        Pos3{0.0f, +14.0f, 2.4f},
        Dir3{0.0f, -28.0f, -2.0f},
        k_quaternion_identity,
        Color3{0.95f, 0.85f, 0.35f},
        "Projectile Yellow"
    );

    // e.create_pyramid(16);
    e.create_pyramid_3d(10, 1.06f, 0.5f);

    e.world.spawn(
        EntityType::Cube,
        Transform{
            .position = Pos3{10.0f, -15.0f, -5.5f},
            .scale = Dir3{10.0f, 10.0f, 1.0f},
        },
        Color3{0.12f, 0.12f, 0.12f}
    );

    static Pos3 origin{0.0f, 0.0f, 0.0f};
    static Pos3 pos{-20.0f, -10.0f, 10.0f};

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
    repulse_pivot.target = &e.world.editor_state().camera().pivot;

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

    spawn_cube(
        e,
        Pos3{-12.0f, 0.0f, 10.0f},
        Dir3{+24.0f, 0.0f, 0.0f},
        k_quaternion_identity,
        Color3{0.90f, 0.35f, 0.35f}
    );
    spawn_cube(
        e,
        Pos3{+12.0f, 0.0f, 10.2f},
        Dir3{-24.0f, 0.0f, 0.0f},
        k_quaternion_identity,
        Color3{0.35f, 0.90f, 0.35f}
    );
    spawn_cube(
        e,
        Pos3{0.0f, -12.0f, 10.0f},
        Dir3{0.0f, +24.0f, 0.0f},
        k_quaternion_identity,
        Color3{0.35f, 0.55f, 0.95f}
    );
    spawn_cube(
        e,
        Pos3{0.0f, +12.0f, 10.4f},
        Dir3{0.0f, -24.0f, -2.0f},
        k_quaternion_identity,
        Color3{0.95f, 0.85f, 0.35f}
    );

    e.create_pyramid(8, 1.06f, 7.0f);
}

void setup_scene_attractor_origin_no_gravity(EngineContext& e) noexcept
{
    static Pos3 origin{0.0f, 0.0f, 0.0f};
    static AttractorForce a{
        .target = &origin,
        .accel_mag = 14.0f,
        .min_radius = 0.9f,
    };
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_attractor_force, .user = &a});

    constexpr auto n = 30;
    const auto r = 16.0f;
    const auto vmag = 7.0f;

    for (int i{0}; i < n; ++i)
    {
        const auto t = static_cast<f32>(i) / static_cast<f32>(n);
        const auto ang = t * k_two_pi;

        const Pos3 p{r * std::cos(ang), r * std::sin(ang), 2.0f};
        const Dir3 v{vmag * (-std::sin(ang)), vmag * (std::cos(ang)), 0.0f};

        spawn_cube(e, p, v, k_quaternion_identity, Color3{0.82f, 0.82f, 0.86f});
    }
}

void setup_scene_attractor_origin_with_gravity(EngineContext& e) noexcept
{
    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    static Pos3 origin{0.0f, 0.0f, 0.0f};
    static AttractorForce a{
        .target = &origin,
        .accel_mag = 10.0f,
        .min_radius = 1.0f,
    };
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_attractor_force, .user = &a});

    constexpr auto n = 26;
    const auto r = 14.0f;
    const auto vmag = 6.0f;

    for (int i = 0; i < n; ++i)
    {
        const auto t = static_cast<f32>(i) / static_cast<f32>(n);
        const auto ang = t * k_two_pi;

        const Pos3 p{r * std::cos(ang), r * std::sin(ang), 12.0f + 0.15f * static_cast<f32>(i)};
        const Dir3 v{vmag * (-std::sin(ang)), vmag * (std::cos(ang)), 0.0f};

        spawn_cube(e, p, v, k_quaternion_identity, Color3{0.86f, 0.86f, 0.86f});
    }
}

void setup_scene_large_pyramid15_ground_gravity(EngineContext& e) noexcept
{
    spawn_static_ground(e, Pos3{0.0f, 0.0f, -3.5f}, Dir3{40.0f, 40.0f, 0.5f});

    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    e.create_pyramid(15, 1.06f, -2.5f);
}

void setup_scene_pyramid3d_heavy_cube_drop(EngineContext& e) noexcept
{
    spawn_static_ground(e, Pos3{0.0f, 0.0f, -3.5f}, Dir3{45.0f, 45.0f, 0.5f});

    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    create_pyramid_3d(e, 7, 1.06f, -2.5f);

    constexpr auto heavy_mass = 350.0f;
    (void) spawn_box(
        e,
        Pos3{1.5f, 0.0f, 18.0f},
        Dir3{2.8f, 2.8f, 2.8f},
        1.0f / heavy_mass,
        Dir3{-1.0f, 0.0f, 0.0f},
        k_quaternion_identity,
        Dir3{0.0f, 0.0f, 0.4f},
        Color3{0.90f, 0.25f, 0.25f},
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
        const auto i_f = static_cast<f32>(i);
        const Pos3 pos{-12.0f + 4.8f * i_f, 0.0f, 2.0f + 0.25f * i_f};
        const Dir3 half_extents{3.0f, 0.25f, 0.25f};
        const auto inv_mass = 1.0f / 8.0f;
        const Dir3 velo{k_zero_dir};
        const Quaternion ori =
            (i % 2 == 0) ? k_quaternion_identity : glm::angleAxis(0.30f * k_pi, k_axis_z);
        const Dir3 angular_velocty{k_zero_dir};

        const EntityId id = spawn_box(
            e,
            pos,
            half_extents,
            inv_mass,
            velo,
            ori,
            angular_velocty,
            Color3{0.80f, 0.82f, 0.88f},
            std::format("Motor Rod {}", i)
        );

        pack.motors[pack.count] = Motor{
            .id = id,
            .torque = Dir3{0.0f, 0.0f, (i % 2 == 0) ? 70.0f : -70.0f},
        };
        e.physics.external_forces.push_back(
            ExternalForce{.fn = apply_motor_torque, .user = &pack.motors[pack.count]}
        );
        ++pack.count;
    }

    for (int i = 0; i < 20; ++i)
    {
        const auto y = -9.0f + 0.95f * static_cast<f32>(i);
        spawn_cube(
            e, Pos3{6.0f, y, 2.0f}, k_zero_dir, k_quaternion_identity, Color3{0.45f, 0.70f, 0.95f}
        );
    }
}

void setup_scene_nbody_sun_3_planets(EngineContext& e) noexcept
{
    static NBodyForceParams params{
        .G = 3.0f,
        .softening = 0.6f,
    };

    e.physics.external_forces.push_back(ExternalForce{.fn = apply_keep_awake, .user = nullptr});
    e.physics.external_forces.push_back(
        ExternalForce{.fn = apply_nbody_gravity_fixed, .user = &params}
    );

    constexpr auto sun_mass = 1200.0f;
    const EntityId sun_id = spawn_box(
        e,
        Pos3{0.0f, 0.0f, 0.0f},
        Dir3{2.2f, 2.2f, 2.2f},
        1.0f / sun_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        Color3{0.95f, 0.75f, 0.25f},
        "Sun"
    );

    const auto r1 = 20.0f;
    const auto r2 = 30.0f;
    const auto r3 = 40.0f;

    const auto v1 = std::sqrt(params.G * sun_mass / r1);
    const auto v2 = std::sqrt(params.G * sun_mass / r2);
    const auto v3 = std::sqrt(params.G * sun_mass / r3);

    spawn_cube(
        e,
        Pos3{r1, 0.0f, 0.0f},
        Dir3{0.0f, v1, 0.0f},
        k_quaternion_identity,
        Color3{0.35f, 0.65f, 0.95f},
        "Planet A"
    );

    spawn_cube(
        e,
        Pos3{0.0f, r2, 0.0f},
        Dir3{-v2, 0.0f, 0.0f},
        k_quaternion_identity,
        Color3{0.35f, 0.95f, 0.55f},
        "Planet B"
    );

    spawn_cube(
        e,
        Pos3{-r3, 0.0f, 0.0f},
        Dir3{0.0f, -v3, 0.0f},
        k_quaternion_identity,
        Color3{0.95f, 0.35f, 0.75f},
        "Planet C"
    );

    const Dir3 p_total{v2, v1 - v3, 0.0f};
    const Dir3 v_sun = -(p_total / sun_mass);

    for (auto& b : e.physics.bodies)
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

    const std::array<Pos3, 3> pos = {
        Pos3{+R, 0.0f, 0.0f},
        Pos3{-0.5f * R, +y, 0.0f},
        Pos3{-0.5f * R, -y, 0.0f},
    };

    const auto v = std::sqrt(p.G * m / (std::sqrt(3.0f) * R));

    for (int i{0}; i < 3; ++i)
    {
        const Dir3 r{pos[static_cast<usize>(i)]};
        const Dir3 t{tangent_ccw_xy(r)};

        (void) spawn_box(
            e,
            pos[static_cast<usize>(i)],
            Dir3{0.8f, 0.8f, 0.8f},
            inv_m,
            (i == 2) ? 1.03f * v * t : v * t,
            k_quaternion_identity,
            k_zero_dir,
            Color3{0.85f, 0.85f, 0.90f},
            std::format("Body {}", i)
        );
    }
}

void setup_scene_moving_attractor_circle(EngineContext& e) noexcept
{
    constexpr int n = 42;
    for (int i = 0; i < n; ++i)
    {
        const auto t = static_cast<f32>(i) / static_cast<f32>(n);
        const auto ang = t * k_two_pi;

        const auto r = 10.0f + 0.25f * static_cast<f32>(i);
        const Pos3 p{
            r * std::cos(ang),
            r * std::sin(ang),
            2.0f + 0.15f * std::sin(3.0f * ang),
        };
        const Dir3 v{
            -2.0f * std::sin(ang),
            +2.0f * std::cos(ang),
            0.0f,
        };

        spawn_cube(e, p, v, k_quaternion_identity, Color3{0.80f, 0.85f, 0.90f});
    }

    static MovingAttractor m{.time_s = 0.0f, .omega = 0.7f, .radius = 18.0f, .z = 2.0f};

    m.attractor = AttractorForce{
        .target = &m.target,
        .accel_mag = 18.0f,
        .min_radius = 1.0f,
    };

    e.physics.external_forces.push_back(ExternalForce{.fn = apply_moving_attractor, .user = &m});
}

void setup_scene_oscillating_uniform_force(EngineContext& e) noexcept
{
    spawn_static_ground(e, Pos3{0.0f, 0.0f, -3.5f}, Dir3{35.0f, 35.0f, 0.5f});

    for (int i = 0; i < 12; ++i)
    {
        spawn_cube(e, Pos3{0.0f, 0.0f, -2.5f + 1.02f * static_cast<f32>(i)});
    }

    static OscillatingUniform f{
        .time_s = 0.0f, .omega = 1.3f, .base_accel = k_gravity, .amp_xy = 10.0f
    };

    e.physics.external_forces.push_back(ExternalForce{.fn = apply_oscillating_uniform, .user = &f});
}

void setup_scene_inclined_plane(EngineContext& e) noexcept
{
    spawn_static_ground(e, Pos3{0.0f, 0.0f, -4.0f}, Dir3{50.0f, 50.0f, 0.5f});

    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    const Quaternion ramp_q = glm::angleAxis(-0.22f * k_pi, k_axis_y);
    (void) spawn_box(
        e,
        Pos3{-6.0f, 0.0f, -1.5f},
        Dir3{18.0f, 6.0f, 0.6f},
        k_static_mass,
        k_zero_dir,
        ramp_q,
        k_zero_dir,
        Color3{0.22f, 0.22f, 0.22f},
        "Ramp"
    );

    for (int i{0}; i < 24; ++i)
    {
        spawn_cube(
            e,
            Pos3{
                -18.0f + 1.2f * static_cast<f32>(i),
                0.0f,
                7.0f + 0.35f * static_cast<f32>(i),
            }
        );
    }
}

void setup_scene_box_drop_container(EngineContext& e) noexcept
{
    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    spawn_static_ground(e, Pos3{0.0f, 0.0f, -3.5f}, Dir3{22.0f, 22.0f, 0.5f});

    const auto wall_th = 0.6f;
    const auto wall_hz = 9.0f;
    const auto inner = 17.0f;
    const auto wall_cz = -3.0f + wall_hz;

    (void) spawn_box(
        e,
        Pos3{+(inner + wall_th), 0.0f, wall_cz},
        Dir3{wall_th, inner, wall_hz},
        k_static_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        Color3{0.12f, 0.12f, 0.12f},
        "+X Wall"
    );
    (void) spawn_box(
        e,
        Pos3{-(inner + wall_th), 0.0f, wall_cz},
        Dir3{wall_th, inner, wall_hz},
        k_static_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        Color3{0.12f, 0.12f, 0.12f},
        "-X Wall"
    );
    (void) spawn_box(
        e,
        Pos3{0.0f, +(inner + wall_th), wall_cz},
        Dir3{inner, wall_th, wall_hz},
        k_static_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        Color3{0.12f, 0.12f, 0.12f},
        "+Y Wall"
    );
    (void) spawn_box(
        e,
        Pos3{0.0f, -(inner + wall_th), wall_cz},
        Dir3{inner, wall_th, wall_hz},
        k_static_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        Color3{0.12f, 0.12f, 0.12f},
        "-Y Wall"
    );

    for (int i = 0; i < 80; ++i)
    {
        const auto fx = static_cast<f32>((i * 37) % 100) / 100.0f;
        const auto fy = static_cast<f32>((i * 73) % 100) / 100.0f;

        const auto x = (fx * 2.0f - 1.0f) * (inner - 2.5f);
        const auto y = (fy * 2.0f - 1.0f) * (inner - 2.5f);
        const auto z = 2.0f + 0.32f * static_cast<f32>(i);
        spawn_cube(e, Pos3{x, y, z});
    }
}

void setup_scene_projectile_wall(EngineContext& e) noexcept
{
    static UniformForce gravity{.accel = k_gravity};
    e.physics.external_forces.push_back(ExternalForce{.fn = apply_uniform_force, .user = &gravity});

    spawn_static_ground(e, Pos3{0.0f, 0.0f, -3.5f}, Dir3{45.0f, 45.0f, 0.5f});

    const auto nx = 14;
    const auto nz = 9;
    const auto step = 1.02f;
    const auto x0 = -0.5f * (nx - 1) * step;
    const auto z0 = -2.5f;

    for (int iz = 0; iz < nz; ++iz)
    {
        for (int ix = 0; ix < nx; ++ix)
        {
            spawn_cube(
                e, Pos3{x0 + static_cast<f32>(ix) * step, 0.0f, z0 + static_cast<f32>(iz) * step}
            );
        }
    }

    spawn_cube(
        e,
        Pos3{0.0f, -22.0f, 2.5f},
        Dir3{0.0f, +36.0f, 0.0f},
        k_quaternion_identity,
        Color3{0.95f, 0.35f, 0.35f},
        "Wall Projectile"
    );
}

}  // namespace ds_pba
