// pba/simulation/scenes_setup.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/core/constants.hpp"
#include "pba/core/math_types.hpp"
#include "pba/physics/forces.hpp"
#include "pba/scene/entity.hpp"
#include "pba/scene/entity_id.hpp"
#include "pba/simulation/simulation_context.hpp"
//
#include <cmath>
#include <string_view>
//
#include <glm/ext/quaternion_trigonometric.hpp>

namespace ds_pba
{
namespace
{

[[nodiscard]] auto spawn_box(
    SimulationContext& sim,
    Pos3 pos,
    Dir3 half_extents,
    f32 inv_mass,
    Dir3 vel = k_zero_dir,
    Quaternion ori = k_quaternion_identity,
    Dir3 ang_vel = k_zero_dir,
    Color3 color = Color3{k_scene_object_default_color},
    std::string_view name = {}
) noexcept -> EntityId
{
    Entity& ent = sim.spawn_cube(pos, half_extents, inv_mass, vel, ori, ang_vel, color, name);
    return ent.id;
}

[[nodiscard]] auto spawn_cube(
    SimulationContext& sim,
    Pos3 pos,
    Dir3 vel = k_zero_dir,
    Quaternion ori = k_quaternion_identity,
    Color3 color = Color3{k_scene_object_default_color},
    std::string_view name = {}
) noexcept -> EntityId
{
    constexpr Dir3 k_half_extents{0.5f, 0.5f, 0.5f};
    constexpr f32 k_inv_mass{1.0f};
    return spawn_box(sim, pos, k_half_extents, k_inv_mass, vel, ori, k_zero_dir, color, name);
}

auto spawn_static_box(
    SimulationContext& sim,
    Pos3 center,
    Dir3 half_extents,
    Color3 color = Color3{0.10f, 0.10f, 0.10f},
    std::string_view name = {}
) noexcept -> void
{
    (void) spawn_box(
        sim,
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

auto spawn_static_ground(
    SimulationContext& sim,
    Pos3 center,
    Dir3 half_extents,
    Color3 color = Color3{0.10f, 0.10f, 0.10f},
    std::string_view name = "Ground"
) noexcept -> void
{
    spawn_static_box(sim, center, half_extents, color, name);
}

auto spawn_open_container(
    SimulationContext& sim,
    f32 inner_half_x,
    f32 inner_half_y,
    f32 floor_top_z,
    f32 wall_half_height,
    f32 wall_half_thickness,
    Color3 color = Color3{0.12f, 0.12f, 0.12f}
) noexcept -> void
{
    constexpr f32 floor_half_z = 0.6f;
    const f32 floor_center_z = floor_top_z - floor_half_z;
    const f32 wall_center_z = floor_top_z + wall_half_height;

    spawn_static_box(
        sim,
        Pos3{0.0f, 0.0f, floor_center_z},
        Dir3{
            inner_half_x + wall_half_thickness,
            inner_half_y + wall_half_thickness,
            floor_half_z,
        },
        color,
        "Container Floor"
    );

    spawn_static_box(
        sim,
        Pos3{+(inner_half_x + wall_half_thickness), 0.0f, wall_center_z},
        Dir3{wall_half_thickness, inner_half_y, wall_half_height},
        color,
        "+X Wall"
    );
    spawn_static_box(
        sim,
        Pos3{-(inner_half_x + wall_half_thickness), 0.0f, wall_center_z},
        Dir3{wall_half_thickness, inner_half_y, wall_half_height},
        color,
        "-X Wall"
    );
    spawn_static_box(
        sim,
        Pos3{0.0f, +(inner_half_y + wall_half_thickness), wall_center_z},
        Dir3{inner_half_x, wall_half_thickness, wall_half_height},
        color,
        "+Y Wall"
    );
    spawn_static_box(
        sim,
        Pos3{0.0f, -(inner_half_y + wall_half_thickness), wall_center_z},
        Dir3{inner_half_x, wall_half_thickness, wall_half_height},
        color,
        "-Y Wall"
    );
}

[[nodiscard]] auto lerp_color(const Color3& a, const Color3& b, f32 t) noexcept -> Color3
{
    const auto u = std::clamp(t, 0.0f, 1.0f);
    return Color3{
        (1.0f - u) * a.r() + u * b.r(),
        (1.0f - u) * a.g() + u * b.g(),
        (1.0f - u) * a.b() + u * b.b(),
    };
}

auto spawn_pyramid_2d(
    SimulationContext& sim,
    Pos3 center,
    int base_n,
    f32 step,
    Color3 low_color,
    Color3 high_color
) noexcept -> void
{
    Expects(base_n > 0);

    for (int layer{0}; layer < base_n; ++layer)
    {
        const int n = base_n - layer;
        const f32 z = center.z + static_cast<f32>(layer) * step;
        const f32 half_span = 0.5f * static_cast<f32>(n - 1) * step;
        const f32 t = static_cast<f32>(layer) / static_cast<f32>(std::max(base_n - 1, 1));

        for (int i{0}; i < n; ++i)
        {
            const f32 x = center.x + static_cast<f32>(i) * step - half_span;
            (void) spawn_cube(
                sim,
                Pos3{x, center.y, z},
                k_zero_dir,
                k_quaternion_identity,
                lerp_color(low_color, high_color, t),
                "Pyramid 2D"
            );
        }
    }
}

auto spawn_pyramid_3d(
    SimulationContext& sim,
    Pos3 center,
    int base_n,
    f32 step,
    Color3 low_color,
    Color3 high_color
) noexcept -> void
{
    Expects(base_n > 0);

    for (int layer{0}; layer < base_n; ++layer)
    {
        const int n = base_n - layer;
        const f32 z = center.z + static_cast<f32>(layer) * step;
        const f32 half_span = 0.5f * static_cast<f32>(n - 1) * step;
        const f32 t = static_cast<f32>(layer) / static_cast<f32>(std::max(base_n - 1, 1));

        for (int ix{0}; ix < n; ++ix)
        {
            const f32 x = center.x + static_cast<f32>(ix) * step - half_span;
            for (int iy{0}; iy < n; ++iy)
            {
                const f32 y = center.y + static_cast<f32>(iy) * step - half_span;
                (void) spawn_cube(
                    sim,
                    Pos3{x, y, z},
                    k_zero_dir,
                    k_quaternion_identity,
                    lerp_color(low_color, high_color, t),
                    "Pyramid 3D"
                );
            }
        }
    }
}

[[nodiscard]] auto hash_to_unit(u32 seed) noexcept -> f32
{
    seed ^= seed >> 16;
    seed *= 0x7FEB352Du;
    seed ^= seed >> 15;
    seed *= 0x846CA68Bu;
    seed ^= seed >> 16;

    constexpr f32 k_inv_24bit = 1.0f / static_cast<f32>(0x01000000u);
    return static_cast<f32>(seed & 0x00FFFFFFu) * k_inv_24bit;
}

}  // namespace

auto setup_scene_stable_pyramid_2d_3d(SimulationContext& sim) noexcept -> void
{
    sim.physics.simple_forces.push_back(SimpleForce{GravityForce{.accel = k_gravity}});

    spawn_static_ground(
        sim,
        Pos3{0.0f, 0.0f, -4.0f},
        Dir3{55.0f, 55.0f, 0.6f},
        Color3{0.09f, 0.09f, 0.09f},
        "Ground"
    );

    spawn_static_box(
        sim,
        Pos3{0.0f, 0.0f, 0.5f},
        Dir3{0.45f, 20.0f, 4.5f},
        Color3{0.14f, 0.14f, 0.14f},
        "Center Divider"
    );

    constexpr f32 base_z = -2.9f;
    constexpr f32 step = 1.02f;

    spawn_pyramid_2d(
        sim,
        Pos3{-16.5f, 0.0f, base_z},
        13,
        step,
        Color3{0.36f, 0.68f, 0.95f},
        Color3{0.90f, 0.95f, 1.0f}
    );

    spawn_pyramid_3d(
        sim,
        Pos3{16.5f, 0.0f, base_z},
        7,
        step,
        Color3{0.95f, 0.64f, 0.38f},
        Color3{1.0f, 0.95f, 0.84f}
    );
}

auto setup_scene_projectile_wall(SimulationContext& sim) noexcept -> void
{
    sim.physics.simple_forces.push_back(SimpleForce{GravityForce{.accel = k_gravity}});

    spawn_static_ground(
        sim,
        Pos3{0.0f, 0.0f, -4.0f},
        Dir3{60.0f, 60.0f, 0.6f},
        Color3{0.09f, 0.09f, 0.09f},
        "Ground"
    );

    constexpr int rows = 12;
    constexpr int cols = 18;
    constexpr f32 step = 1.02f;
    constexpr f32 wall_x = 12.0f;
    constexpr f32 wall_base_z = -2.9f;
    const f32 y0 = -0.5f * static_cast<f32>(cols - 1) * step;

    for (int row{0}; row < rows; ++row)
    {
        const f32 z = wall_base_z + static_cast<f32>(row) * step;
        const f32 row_offset = (row % 2 == 0) ? 0.0f : 0.5f * step;
        const f32 t = static_cast<f32>(row) / static_cast<f32>(rows - 1);

        for (int col{0}; col < cols; ++col)
        {
            const f32 y = y0 + static_cast<f32>(col) * step + row_offset;
            (void) spawn_cube(
                sim,
                Pos3{wall_x, y, z},
                k_zero_dir,
                k_quaternion_identity,
                lerp_color(Color3{0.72f, 0.74f, 0.79f}, Color3{0.86f, 0.89f, 0.94f}, t),
                "Wall Brick"
            );
        }
    }

    constexpr f32 heavy_mass = 48.0f;
    (void) spawn_box(
        sim,
        Pos3{-34.0f, 0.0f, 0.2f},
        Dir3{1.2f, 1.2f, 1.2f},
        1.0f / heavy_mass,
        Dir3{58.0f, 0.0f, 1.5f},
        k_quaternion_identity,
        Dir3{0.0f, 0.0f, 1.0f},
        Color3{0.95f, 0.30f, 0.28f},
        "Primary Projectile"
    );

    constexpr f32 scout_mass = 12.0f;
    (void) spawn_box(
        sim,
        Pos3{-32.0f, -6.0f, 2.6f},
        Dir3{0.75f, 0.75f, 0.75f},
        1.0f / scout_mass,
        Dir3{54.0f, 8.0f, -0.4f},
        k_quaternion_identity,
        Dir3{0.0f, 0.0f, -4.0f},
        Color3{0.98f, 0.62f, 0.25f},
        "Secondary Projectile"
    );
}

auto setup_scene_cube_cloud_1200(SimulationContext& sim) noexcept -> void
{
    sim.physics.simple_forces.push_back(SimpleForce{GravityForce{.accel = k_gravity}});

    constexpr f32 floor_top_z = -3.2f;
    spawn_open_container(
        sim,
        14.5f,
        14.5f,
        floor_top_z,
        12.0f,
        0.7f,
        Color3{0.10f, 0.10f, 0.12f}
    );

    constexpr int nx = 12;
    constexpr int ny = 10;
    constexpr int nz = 10;
    static_assert(nx * ny * nz == 1200, "Cube cloud scene must spawn exactly 1200 cubes.");

    constexpr f32 step = 1.04f;
    const f32 x0 = -0.5f * static_cast<f32>(nx - 1) * step;
    const f32 y0 = -0.5f * static_cast<f32>(ny - 1) * step;
    const f32 z0 = floor_top_z + 0.55f;

    for (int iz{0}; iz < nz; ++iz)
    {
        const f32 layer_t = static_cast<f32>(iz) / static_cast<f32>(nz - 1);
        const Color3 layer_color =
            lerp_color(Color3{0.56f, 0.74f, 0.95f}, Color3{0.95f, 0.74f, 0.48f}, layer_t);

        for (int iy{0}; iy < ny; ++iy)
        {
            for (int ix{0}; ix < nx; ++ix)
            {
                const u32 seed = static_cast<u32>(ix + 1) * 73856093u
                    ^ static_cast<u32>(iy + 1) * 19349663u ^ static_cast<u32>(iz + 1) * 83492791u;

                const f32 jx = (hash_to_unit(seed) - 0.5f) * 0.14f;
                const f32 jy = (hash_to_unit(seed ^ 0xB5297A4Du) - 0.5f) * 0.14f;
                const f32 jz = (hash_to_unit(seed ^ 0x68E31DA4u) - 0.5f) * 0.04f;

                const f32 spin = static_cast<f32>(ix + 7 * iy + 13 * iz);
                const Dir3 vel{
                    0.60f * std::cos(0.21f * spin),
                    0.60f * std::sin(0.19f * spin),
                    0.0f,
                };

                const Pos3 pos{
                    x0 + static_cast<f32>(ix) * step + jx,
                    y0 + static_cast<f32>(iy) * step + jy,
                    z0 + static_cast<f32>(iz) * step + jz,
                };

                (void) spawn_cube(sim, pos, vel, k_quaternion_identity, layer_color, "Cloud Cube");
            }
        }
    }
}

auto setup_scene_orbital_rotor_vortex(SimulationContext& sim) noexcept -> void
{
    spawn_static_box(
        sim,
        Pos3{0.0f, 0.0f, -8.6f},
        Dir3{24.0f, 24.0f, 0.6f},
        Color3{0.07f, 0.07f, 0.08f},
        "Chamber Floor"
    );
    spawn_static_box(
        sim,
        Pos3{0.0f, 0.0f, +8.6f},
        Dir3{24.0f, 24.0f, 0.6f},
        Color3{0.07f, 0.07f, 0.08f},
        "Chamber Ceiling"
    );

    constexpr f32 wall_half_th = 0.6f;
    constexpr f32 inner_half = 24.0f;
    constexpr f32 wall_half_h = 8.0f;

    spawn_static_box(
        sim,
        Pos3{+(inner_half + wall_half_th), 0.0f, 0.0f},
        Dir3{wall_half_th, inner_half, wall_half_h},
        Color3{0.09f, 0.09f, 0.10f},
        "+X Wall"
    );
    spawn_static_box(
        sim,
        Pos3{-(inner_half + wall_half_th), 0.0f, 0.0f},
        Dir3{wall_half_th, inner_half, wall_half_h},
        Color3{0.09f, 0.09f, 0.10f},
        "-X Wall"
    );
    spawn_static_box(
        sim,
        Pos3{0.0f, +(inner_half + wall_half_th), 0.0f},
        Dir3{inner_half, wall_half_th, wall_half_h},
        Color3{0.09f, 0.09f, 0.10f},
        "+Y Wall"
    );
    spawn_static_box(
        sim,
        Pos3{0.0f, -(inner_half + wall_half_th), 0.0f},
        Dir3{inner_half, wall_half_th, wall_half_h},
        Color3{0.09f, 0.09f, 0.10f},
        "-Y Wall"
    );

    const auto spawn_rotor = [&](f32 z, f32 angle, f32 torque_z, Color3 color, std::string_view name)
    {
        constexpr f32 rotor_mass = 220.0f;
        const EntityId id = spawn_box(
            sim,
            Pos3{0.0f, 0.0f, z},
            Dir3{7.2f, 0.35f, 0.35f},
            1.0f / rotor_mass,
            k_zero_dir,
            glm::angleAxis(angle, k_axis_z),
            k_zero_dir,
            color,
            name
        );

        sim.physics.simple_forces.push_back(SimpleForce{MotorForce{
            .id = id,
            .torque = Dir3{0.0f, 0.0f, torque_z},
        }});
    };

    spawn_rotor(-1.4f, 0.0f, +220.0f, Color3{0.94f, 0.36f, 0.34f}, "Rotor A");
    spawn_rotor(0.0f, (2.0f / 3.0f) * k_pi, -220.0f, Color3{0.34f, 0.88f, 0.66f}, "Rotor B");
    spawn_rotor(+1.4f, (4.0f / 3.0f) * k_pi, +220.0f, Color3{0.36f, 0.58f, 0.96f}, "Rotor C");

    const Pos3 center{0.0f, 0.0f, 0.0f};
    sim.physics.simple_forces.push_back(SimpleForce{AttractorForce{
        .target = center,
        .magnitude = 11.5f,
        .min_radius = 3.0f,
    }});
    sim.physics.simple_forces.push_back(SimpleForce{RepulsionForce{
        .target = center,
        .accel_max = 42.0f,
        .range = 4.8f,
        .min_radius = 1.4f,
    }});

    constexpr int ring_count = 140;
    for (int i{0}; i < ring_count; ++i)
    {
        const f32 t = static_cast<f32>(i) / static_cast<f32>(ring_count);
        const f32 ang = t * k_two_pi;

        const f32 radius = 15.0f + 2.0f * std::sin(3.0f * ang);
        const Pos3 p{
            radius * std::cos(ang),
            radius * std::sin(ang),
            2.6f * std::sin(2.0f * ang),
        };

        const Dir3 tangent{-std::sin(ang), std::cos(ang), 0.0f};
        const Dir3 radial{std::cos(ang), std::sin(ang), 0.0f};

        const Dir3 v = 8.6f * tangent + 0.9f * radial
            + Dir3{0.0f, 0.0f, 1.2f * std::cos(5.0f * ang)};

        (void) spawn_cube(
            sim,
            p,
            v,
            k_quaternion_identity,
            lerp_color(Color3{0.72f, 0.79f, 0.98f}, Color3{0.96f, 0.66f, 0.45f}, t),
            "Vortex Orbiter"
        );
    }

    constexpr int inner_count = 24;
    for (int i{0}; i < inner_count; ++i)
    {
        const f32 t = static_cast<f32>(i) / static_cast<f32>(inner_count);
        const f32 ang = t * k_two_pi;

        constexpr f32 orbit_r = 5.5f;
        const Pos3 p{
            orbit_r * std::cos(ang),
            orbit_r * std::sin(ang),
            static_cast<f32>((i % 3) - 1) * 0.9f,
        };

        const Dir3 tangent{-std::sin(ang), std::cos(ang), 0.0f};
        const Dir3 v = -5.0f * tangent;

        (void) spawn_box(
            sim,
            p,
            Dir3{0.75f, 0.75f, 0.75f},
            1.0f / 8.0f,
            v,
            k_quaternion_identity,
            k_zero_dir,
            Color3{0.86f, 0.90f, 0.96f},
            "Inner Orbiter"
        );
    }
}

auto setup_scene_domino_spiral_cascade(SimulationContext& sim) noexcept -> void
{
    sim.physics.simple_forces.push_back(SimpleForce{GravityForce{.accel = k_gravity}});

    spawn_static_ground(
        sim,
        Pos3{0.0f, 0.0f, -4.0f},
        Dir3{65.0f, 65.0f, 0.6f},
        Color3{0.08f, 0.08f, 0.09f},
        "Ground"
    );

    constexpr Dir3 domino_half_extents{0.12f, 0.44f, 1.10f};
    constexpr f32 domino_z = -3.4f + domino_half_extents.z;
    constexpr f32 spacing = 0.96f;
    constexpr int loops = 5;

    auto domino_idx = usize{0zu};
    auto emit_domino = [&](Pos3 p, f32 yaw)
    {
        const auto t = static_cast<f32>((domino_idx % 96zu)) / 95.0f;
        const Color3 color = lerp_color(Color3{0.58f, 0.74f, 0.96f}, Color3{0.96f, 0.62f, 0.34f}, t);

        Quaternion q = glm::angleAxis(yaw, k_axis_z);
        if (domino_idx == 0zu)
        {
            // Start the cascade by pre-tilting the very first domino.
            const Quaternion tilt = glm::angleAxis(-0.08f * k_pi, k_axis_x);
            q = glm::normalize(tilt * q);
        }

        (void) spawn_box(
            sim,
            p,
            domino_half_extents,
            1.0f,
            k_zero_dir,
            q,
            k_zero_dir,
            color,
            "Domino"
        );
        ++domino_idx;
    };

    f32 half = 24.0f;
    for (int ring{0}; ring < loops; ++ring)
    {
        for (f32 y = -half; y <= half; y += spacing)
        {
            emit_domino(Pos3{-half, y, domino_z}, +0.5f * k_pi);
        }
        for (f32 x = -half + spacing; x <= half; x += spacing)
        {
            emit_domino(Pos3{x, +half, domino_z}, 0.0f);
        }
        for (f32 y = +half - spacing; y >= -half; y -= spacing)
        {
            emit_domino(Pos3{+half, y, domino_z}, -0.5f * k_pi);
        }
        for (f32 x = +half - spacing; x >= -half + spacing; x -= spacing)
        {
            emit_domino(Pos3{x, -half, domino_z}, k_pi);
        }

        half -= 3.8f;
    }
}

auto setup_scene_tumbler_drum(SimulationContext& sim) noexcept -> void
{
    sim.physics.simple_forces.push_back(SimpleForce{GravityForce{.accel = k_gravity}});

    constexpr f32 floor_top_z = -3.2f;
    spawn_open_container(
        sim,
        13.5f,
        13.5f,
        floor_top_z,
        10.0f,
        0.7f,
        Color3{0.09f, 0.09f, 0.10f}
    );

    const auto spawn_paddle = [&](f32 angle, f32 z, f32 torque, std::string_view name)
    {
        constexpr f32 paddle_mass = 420.0f;
        const EntityId id = spawn_box(
            sim,
            Pos3{0.0f, 0.0f, z},
            Dir3{6.2f, 0.45f, 0.45f},
            1.0f / paddle_mass,
            k_zero_dir,
            glm::angleAxis(angle, k_axis_z),
            k_zero_dir,
            Color3{0.86f, 0.34f, 0.30f},
            name
        );

        sim.physics.simple_forces.push_back(SimpleForce{MotorForce{
            .id = id,
            .torque = Dir3{0.0f, 0.0f, torque},
        }});
    };

    spawn_paddle(0.0f, -0.2f, +180.0f, "Drum Paddle A");
    spawn_paddle((2.0f / 3.0f) * k_pi, 0.8f, -170.0f, "Drum Paddle B");
    spawn_paddle((4.0f / 3.0f) * k_pi, 1.8f, +165.0f, "Drum Paddle C");

    constexpr int nx = 10;
    constexpr int ny = 8;
    constexpr int nz = 5;
    constexpr f32 step = 1.06f;
    const f32 x0 = -0.5f * static_cast<f32>(nx - 1) * step;
    const f32 y0 = -0.5f * static_cast<f32>(ny - 1) * step;
    const f32 z0 = floor_top_z + 1.0f;

    for (int iz{0}; iz < nz; ++iz)
    {
        const f32 layer_t = static_cast<f32>(iz) / static_cast<f32>(nz - 1);
        for (int iy{0}; iy < ny; ++iy)
        {
            for (int ix{0}; ix < nx; ++ix)
            {
                const u32 seed = static_cast<u32>(ix + 1) * 19937u
                    ^ static_cast<u32>(iy + 1) * 31337u ^ static_cast<u32>(iz + 1) * 65537u;
                const f32 jx = (hash_to_unit(seed) - 0.5f) * 0.10f;
                const f32 jy = (hash_to_unit(seed ^ 0x51ED270Bu) - 0.5f) * 0.10f;

                (void) spawn_cube(
                    sim,
                    Pos3{
                        x0 + static_cast<f32>(ix) * step + jx,
                        y0 + static_cast<f32>(iy) * step + jy,
                        z0 + static_cast<f32>(iz) * step,
                    },
                    k_zero_dir,
                    k_quaternion_identity,
                    lerp_color(Color3{0.62f, 0.76f, 0.95f}, Color3{0.95f, 0.72f, 0.44f}, layer_t),
                    "Drum Cube"
                );
            }
        }
    }
}

auto setup_scene_cup_rain_collapse(SimulationContext& sim) noexcept -> void
{
    sim.physics.simple_forces.push_back(SimpleForce{GravityForce{.accel = k_gravity}});

    spawn_static_ground(
        sim,
        Pos3{0.0f, 0.0f, -4.0f},
        Dir3{58.0f, 58.0f, 0.6f},
        Color3{0.09f, 0.09f, 0.09f},
        "Ground"
    );

    constexpr f32 floor_top = -3.4f;
    constexpr f32 wall_th = 0.7f;
    constexpr f32 inner = 10.5f;
    constexpr f32 tall_h = 11.0f;
    constexpr f32 front_h = 6.5f;

    spawn_static_box(
        sim,
        Pos3{0.0f, 0.0f, floor_top - 0.6f},
        Dir3{inner + wall_th, inner + wall_th, 0.6f},
        Color3{0.12f, 0.12f, 0.12f},
        "Cup Floor"
    );
    spawn_static_box(
        sim,
        Pos3{+(inner + wall_th), 0.0f, floor_top + tall_h},
        Dir3{wall_th, inner, tall_h},
        Color3{0.12f, 0.12f, 0.12f},
        "Cup Right Wall"
    );
    spawn_static_box(
        sim,
        Pos3{-(inner + wall_th), 0.0f, floor_top + tall_h},
        Dir3{wall_th, inner, tall_h},
        Color3{0.12f, 0.12f, 0.12f},
        "Cup Left Wall"
    );
    spawn_static_box(
        sim,
        Pos3{0.0f, +(inner + wall_th), floor_top + tall_h},
        Dir3{inner, wall_th, tall_h},
        Color3{0.12f, 0.12f, 0.12f},
        "Cup Back Wall"
    );
    spawn_static_box(
        sim,
        Pos3{0.0f, -(inner + wall_th), floor_top + front_h},
        Dir3{inner, wall_th, front_h},
        Color3{0.12f, 0.12f, 0.12f},
        "Cup Front Lip"
    );

    constexpr int nx = 12;
    constexpr int ny = 10;
    constexpr int nz = 6;
    constexpr f32 step = 1.06f;

    const f32 x0 = -0.5f * static_cast<f32>(nx - 1) * step;
    const f32 y0 = -0.5f * static_cast<f32>(ny - 1) * step;
    const f32 z0 = 8.0f;

    for (int iz{0}; iz < nz; ++iz)
    {
        const f32 layer_t = static_cast<f32>(iz) / static_cast<f32>(nz - 1);
        for (int iy{0}; iy < ny; ++iy)
        {
            for (int ix{0}; ix < nx; ++ix)
            {
                const u32 seed = static_cast<u32>(ix + 7) * 2654435761u
                    ^ static_cast<u32>(iy + 11) * 2246822519u ^ static_cast<u32>(iz + 3) * 3266489917u;
                const f32 jx = (hash_to_unit(seed) - 0.5f) * 0.26f;
                const f32 jy = (hash_to_unit(seed ^ 0x9E3779B9u) - 0.5f) * 0.26f;
                const f32 drift = (hash_to_unit(seed ^ 0x85EBCA6Bu) - 0.5f) * 1.8f;

                (void) spawn_cube(
                    sim,
                    Pos3{
                        x0 + static_cast<f32>(ix) * step + jx,
                        y0 + static_cast<f32>(iy) * step + jy,
                        z0 + static_cast<f32>(iz) * step,
                    },
                    Dir3{drift, -0.25f * drift, 0.0f},
                    k_quaternion_identity,
                    lerp_color(Color3{0.66f, 0.78f, 0.97f}, Color3{0.96f, 0.70f, 0.43f}, layer_t),
                    "Cup Rain Cube"
                );
            }
        }
    }
}

auto setup_scene_tower_demolition(SimulationContext& sim) noexcept -> void
{
    sim.physics.simple_forces.push_back(SimpleForce{GravityForce{.accel = k_gravity}});

    spawn_static_ground(
        sim,
        Pos3{0.0f, 0.0f, -4.0f},
        Dir3{80.0f, 80.0f, 0.6f},
        Color3{0.08f, 0.08f, 0.09f},
        "Ground"
    );

    constexpr int layers = 28;
    constexpr int half_n = 4;
    constexpr f32 step = 1.02f;
    constexpr Pos3 center{12.0f, 0.0f, -2.9f};

    for (int layer{0}; layer < layers; ++layer)
    {
        const f32 z = center.z + static_cast<f32>(layer) * step;
        const f32 t = static_cast<f32>(layer) / static_cast<f32>(layers - 1);

        for (int ix{-half_n}; ix <= half_n; ++ix)
        {
            for (int iy{-half_n}; iy <= half_n; ++iy)
            {
                const auto boundary = (std::abs(ix) == half_n) || (std::abs(iy) == half_n);
                if (!boundary)
                {
                    continue;
                }
                if (((ix + iy + layer) & 1) != 0)
                {
                    continue;
                }

                (void) spawn_cube(
                    sim,
                    Pos3{
                        center.x + static_cast<f32>(ix) * step,
                        center.y + static_cast<f32>(iy) * step,
                        z,
                    },
                    k_zero_dir,
                    k_quaternion_identity,
                    lerp_color(Color3{0.74f, 0.78f, 0.86f}, Color3{0.92f, 0.94f, 0.98f}, t),
                    "Tower Cube"
                );
            }
        }
    }

    (void) spawn_box(
        sim,
        Pos3{-34.0f, 0.0f, 6.0f},
        Dir3{1.5f, 1.5f, 1.5f},
        1.0f / 70.0f,
        Dir3{62.0f, 0.0f, 0.5f},
        k_quaternion_identity,
        Dir3{0.0f, 0.0f, 0.8f},
        Color3{0.95f, 0.30f, 0.28f},
        "Demolition Shot A"
    );

    (void) spawn_box(
        sim,
        Pos3{56.0f, 0.0f, 14.0f},
        Dir3{1.1f, 1.1f, 1.1f},
        1.0f / 32.0f,
        Dir3{-58.0f, 0.0f, -3.0f},
        k_quaternion_identity,
        Dir3{0.0f, 0.0f, -2.5f},
        Color3{0.98f, 0.62f, 0.25f},
        "Demolition Shot B"
    );
}

auto setup_scene_triple_pyramid_siege(SimulationContext& sim) noexcept -> void
{
    sim.physics.simple_forces.push_back(SimpleForce{GravityForce{.accel = k_gravity}});

    spawn_static_ground(
        sim,
        Pos3{0.0f, 0.0f, -4.0f},
        Dir3{62.0f, 62.0f, 0.6f},
        Color3{0.09f, 0.09f, 0.09f},
        "Ground"
    );

    spawn_pyramid_2d(
        sim,
        Pos3{-16.0f, -10.0f, -2.9f},
        11,
        1.02f,
        Color3{0.44f, 0.72f, 0.96f},
        Color3{0.84f, 0.93f, 1.0f}
    );
    spawn_pyramid_2d(
        sim,
        Pos3{-16.0f, +10.0f, -2.9f},
        11,
        1.02f,
        Color3{0.44f, 0.72f, 0.96f},
        Color3{0.84f, 0.93f, 1.0f}
    );
    spawn_pyramid_3d(
        sim,
        Pos3{14.0f, 0.0f, -2.9f},
        6,
        1.02f,
        Color3{0.95f, 0.66f, 0.40f},
        Color3{1.0f, 0.94f, 0.82f}
    );

    (void) spawn_box(
        sim,
        Pos3{-42.0f, -10.0f, 2.2f},
        Dir3{1.0f, 1.0f, 1.0f},
        1.0f / 30.0f,
        Dir3{64.0f, 0.0f, 0.4f},
        k_quaternion_identity,
        Dir3{0.0f, 0.0f, 0.8f},
        Color3{0.95f, 0.34f, 0.34f},
        "Siege Shot Left"
    );
    (void) spawn_box(
        sim,
        Pos3{-42.0f, +10.0f, 2.2f},
        Dir3{1.0f, 1.0f, 1.0f},
        1.0f / 30.0f,
        Dir3{64.0f, 0.0f, 0.4f},
        k_quaternion_identity,
        Dir3{0.0f, 0.0f, -0.8f},
        Color3{0.95f, 0.34f, 0.34f},
        "Siege Shot Right"
    );

    (void) spawn_box(
        sim,
        Pos3{14.0f, 0.0f, 21.0f},
        Dir3{2.0f, 2.0f, 2.0f},
        1.0f / 240.0f,
        Dir3{-0.8f, 0.0f, -1.0f},
        k_quaternion_identity,
        Dir3{0.0f, 0.0f, 0.3f},
        Color3{0.98f, 0.28f, 0.24f},
        "Siege Drop Weight"
    );
}

auto setup_scene_cube_crossfire_arena(SimulationContext& sim) noexcept -> void
{
    spawn_static_box(
        sim,
        Pos3{0.0f, 0.0f, -10.6f},
        Dir3{26.0f, 26.0f, 0.6f},
        Color3{0.07f, 0.07f, 0.08f},
        "Arena Floor"
    );
    spawn_static_box(
        sim,
        Pos3{0.0f, 0.0f, +10.6f},
        Dir3{26.0f, 26.0f, 0.6f},
        Color3{0.07f, 0.07f, 0.08f},
        "Arena Ceiling"
    );
    spawn_static_box(
        sim,
        Pos3{+26.6f, 0.0f, 0.0f},
        Dir3{0.6f, 26.0f, 10.0f},
        Color3{0.09f, 0.09f, 0.10f},
        "+X Wall"
    );
    spawn_static_box(
        sim,
        Pos3{-26.6f, 0.0f, 0.0f},
        Dir3{0.6f, 26.0f, 10.0f},
        Color3{0.09f, 0.09f, 0.10f},
        "-X Wall"
    );
    spawn_static_box(
        sim,
        Pos3{0.0f, +26.6f, 0.0f},
        Dir3{26.0f, 0.6f, 10.0f},
        Color3{0.09f, 0.09f, 0.10f},
        "+Y Wall"
    );
    spawn_static_box(
        sim,
        Pos3{0.0f, -26.6f, 0.0f},
        Dir3{26.0f, 0.6f, 10.0f},
        Color3{0.09f, 0.09f, 0.10f},
        "-Y Wall"
    );

    for (int iz{-4}; iz <= 4; ++iz)
    {
        for (int iy{-4}; iy <= 4; ++iy)
        {
            if (((iy + iz) & 1) != 0)
            {
                continue;
            }
            spawn_static_box(
                sim,
                Pos3{0.0f, 1.18f * static_cast<f32>(iy), 1.18f * static_cast<f32>(iz)},
                Dir3{0.5f, 0.5f, 0.5f},
                Color3{0.13f, 0.13f, 0.14f},
                "Center Obstacle"
            );
        }
    }

    sim.physics.simple_forces.push_back(SimpleForce{RepulsionForce{
        .target = Pos3{0.0f, 0.0f, 0.0f},
        .accel_max = 12.0f,
        .range = 4.0f,
        .min_radius = 1.0f,
    }});

    constexpr int lanes = 10;
    for (int i{0}; i < lanes; ++i)
    {
        const f32 t = static_cast<f32>(i) / static_cast<f32>(lanes - 1);
        const f32 y = -12.0f + 24.0f * t;
        const f32 z = -3.5f + 7.0f * std::sin(2.0f * k_pi * t);

        (void) spawn_cube(
            sim, Pos3{-22.0f, y, z}, Dir3{24.0f, 0.0f, 0.0f}, k_quaternion_identity, Color3{0.92f, 0.36f, 0.34f}, "Crossfire X-"
        );
        (void) spawn_cube(
            sim, Pos3{+22.0f, y, -z}, Dir3{-24.0f, 0.0f, 0.0f}, k_quaternion_identity, Color3{0.36f, 0.88f, 0.66f}, "Crossfire X+"
        );
        (void) spawn_cube(
            sim, Pos3{y, -22.0f, z}, Dir3{0.0f, 24.0f, 0.0f}, k_quaternion_identity, Color3{0.38f, 0.60f, 0.95f}, "Crossfire Y-"
        );
        (void) spawn_cube(
            sim, Pos3{-y, +22.0f, -z}, Dir3{0.0f, -24.0f, 0.0f}, k_quaternion_identity, Color3{0.95f, 0.80f, 0.32f}, "Crossfire Y+"
        );
    }
}

auto setup_scene_nbody_cube_galaxy(SimulationContext& sim) noexcept -> void
{
    static NBodyForce params{
        .G = 3.1f,
        .softening = 0.7f,
    };
    sim.physics.complex_forces.push_back(ComplexForce{params});

    constexpr f32 core_mass = 2200.0f;
    (void) spawn_box(
        sim,
        Pos3{0.0f, 0.0f, 0.0f},
        Dir3{2.2f, 2.2f, 2.2f},
        1.0f / core_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        Color3{0.95f, 0.76f, 0.26f},
        "Galactic Core"
    );

    const auto spawn_ring = [&](int n, f32 radius, f32 z_amp, Color3 c0, Color3 c1)
    {
        const f32 vmag = std::sqrt(params.G * core_mass / radius);
        for (int i{0}; i < n; ++i)
        {
            const f32 t = static_cast<f32>(i) / static_cast<f32>(n);
            const f32 ang = t * k_two_pi;
            const Pos3 p{
                radius * std::cos(ang),
                radius * std::sin(ang),
                z_amp * std::sin(2.0f * ang),
            };
            const Dir3 tangent{-std::sin(ang), std::cos(ang), 0.0f};
            const Dir3 v{
                vmag * tangent.x,
                vmag * tangent.y,
                0.35f * z_amp * std::cos(2.0f * ang),
            };

            (void) spawn_box(
                sim,
                p,
                Dir3{0.45f, 0.45f, 0.45f},
                1.0f / 4.0f,
                v,
                k_quaternion_identity,
                k_zero_dir,
                lerp_color(c0, c1, t),
                "Galactic Cube"
            );
        }
    };

    spawn_ring(24, 12.0f, 0.6f, Color3{0.40f, 0.70f, 0.95f}, Color3{0.72f, 0.88f, 0.98f});
    spawn_ring(36, 18.0f, 1.1f, Color3{0.40f, 0.95f, 0.64f}, Color3{0.84f, 0.98f, 0.78f});
    spawn_ring(48, 25.0f, 1.6f, Color3{0.95f, 0.62f, 0.40f}, Color3{0.99f, 0.88f, 0.65f});

    constexpr f32 perturber_mass = 380.0f;
    const f32 r = 9.0f;
    const f32 v = std::sqrt(params.G * core_mass / r);
    (void) spawn_box(
        sim,
        Pos3{0.0f, -r, 0.0f},
        Dir3{1.4f, 1.4f, 1.4f},
        1.0f / perturber_mass,
        Dir3{v, 0.0f, 0.0f},
        k_quaternion_identity,
        Dir3{0.0f, 0.0f, 0.2f},
        Color3{0.96f, 0.36f, 0.42f},
        "Galactic Perturber"
    );
}

auto setup_scene_inclined_avalanche(SimulationContext& sim) noexcept -> void
{
    sim.physics.simple_forces.push_back(SimpleForce{GravityForce{.accel = k_gravity}});

    // Broad catch plane for downstream debris.
    spawn_static_box(
        sim,
        Pos3{-12.0f, 3.0f, -30.0f},
        Dir3{140.0f, 80.0f, 1.2f},
        Color3{0.08f, 0.08f, 0.09f},
        "Avalanche Basin"
    );

    constexpr f32 deg = k_pi / 180.0f;
    constexpr f32 small_tilt_deg = 18.0f;
    constexpr f32 big_tilt_deg = 26.0f;

    // Upper feeder slope: keep orientation simple (single-axis tilt) for clean yz alignment.
    const Quaternion small_slope_q = glm::angleAxis(small_tilt_deg * deg, k_axis_x);
    (void) spawn_box(
        sim,
        Pos3{-28.0f, 0.0f, 7.0f},
        Dir3{60.0f, 8.0f, 0.9f},
        k_static_mass,
        k_zero_dir,
        small_slope_q,
        k_zero_dir,
        Color3{0.13f, 0.13f, 0.14f},
        "Avalanche Small Slope"
    );

    // Main slope: also single-axis tilt to preserve the elongated-2D setup.
    const Quaternion big_slope_q = glm::angleAxis(big_tilt_deg * deg, k_axis_x);
    (void) spawn_box(
        sim,
        Pos3{-27.0f, 0.0f, -7.0f},
        Dir3{96.0f, 20.0f, 1.1f},
        k_static_mass,
        k_zero_dir,
        big_slope_q,
        k_zero_dir,
        Color3{0.12f, 0.12f, 0.13f},
        "Avalanche Big Slope"
    );

    // Reduced count, same 6-layer height.
    // Layout is defined in yz and then extruded along x.
    constexpr int nx = 8;
    constexpr int ny = 15;
    constexpr int nz = 6;

    constexpr f32 step_x = 1.00f;
    constexpr f32 step_y = 1.00f;
    constexpr f32 step_z = 1.02f;

    const f32 x0 = -31.5f;
    const f32 y0 = -7.0f;
    const f32 slope_sin = std::sin(small_tilt_deg * deg);
    const f32 slope_cos = std::cos(small_tilt_deg * deg);

    for (int iz{0}; iz < nz; ++iz)
    {
        const f32 layer_t = static_cast<f32>(iz) / static_cast<f32>(nz - 1);
        for (int iy{0}; iy < ny; ++iy)
        {
            const f32 y = y0 + static_cast<f32>(iy) * step_y;

            // Top surface of the small slope at this y (x-independent because we only tilt on x).
            const f32 top_z = 7.0f + y * slope_sin + 0.9f * slope_cos;
            const f32 z_base = top_z + 0.52f;

            for (int ix{0}; ix < nx; ++ix)
            {
                (void) spawn_cube(
                    sim,
                    Pos3{
                        x0 + static_cast<f32>(ix) * step_x,
                        y,
                        z_base + static_cast<f32>(iz) * step_z,
                    },
                    k_zero_dir,
                    k_quaternion_identity,
                    lerp_color(Color3{0.62f, 0.76f, 0.95f}, Color3{0.95f, 0.70f, 0.44f}, layer_t),
                    "Avalanche Cube"
                );
            }
        }
    }
}

}  // namespace ds_pba
