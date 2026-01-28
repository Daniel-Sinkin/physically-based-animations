// pba/engine/engine_context.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine/engine_context.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/engine/scenes.hpp"
#include "pba/scene/entity.hpp"
#include "pba/ui/ui.hpp"
//
#include <algorithm>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <gsl/assert>

namespace ds_pba
{
namespace
{

glm::mat3 inv_inertia_body_box(f32 inv_mass, const Dir3& half_extent) noexcept
{
    if (inv_mass == k_static_mass)
    {
        return glm::mat3(0.0f);
    }
    assert(inv_mass > 0.0f && "Static bodies should have already early returned");
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

glm::mat3
inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
{
    const glm::mat3 R{glm::mat3_cast(q)};
    return R * inv_inertia_body * glm::transpose(R);
}

void init_box_inertia(RigidBody& b) noexcept
{
    b.inv_inertia_body = inv_inertia_body_box(b.inv_mass, b.half_extents);
    b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
}

}  // namespace

void EngineContext::link_latest_objects(EntityId id)
{
    {
        Expects(!physics.bodies.empty());
    }
    obj_map.insert_or_assign(
        id,
        EntityLink{
            world.entities().size() - 1zu,
            physics.bodies.size() - 1zu,
        }
    );
}

void EngineContext::add_cube(Pos3 position)
{
    const auto& entity = world.spawn(EntityType::Cube, Transform{.position = position});
    physics.bodies.push_back(
        RigidBody{
            .id = entity.id,
            .half_extents = Dir3{0.5f, 0.5f, 0.5f},
            .position = position,
            .inv_mass = 1.0f,
        }
    );
    init_box_inertia(physics.bodies.back());
    link_latest_objects(entity.id);
}

void EngineContext::add_ground()
{
    constexpr Pos3 ground_center{0.0f, 0.0f, -3.5f};
    constexpr Pos3 half_extents{10.0f, 10.0f, 0.5f};

    const auto& entity = world.spawn(
        EntityType::Cube,
        Transform{.position = ground_center, .scale = half_extents * 2.0f},
        {0.1f, 0.1f, 0.1f}
    );

    physics.bodies.push_back(
        RigidBody{
            .id = entity.id,

            .half_extents = half_extents,

            .position = ground_center,
            .velocity = Dir3{0.0f, 0.0f, 30.0f},
            .inv_mass = 1.0f / 50.0f,
            .inv_inertia_body = glm::mat3(0.0f),
            .inv_inertia_world = glm::mat3(0.0f),
        }
    );

    init_box_inertia(physics.bodies.back());

    link_latest_objects(entity.id);
    obj_name_map.insert_or_assign(entity.id, "Ground");
}

void EngineContext::spawn_cube(Pos3 pos, Dir3 vel, Quaternion ori, Color3 color)
{
    add_cube(pos);

    RigidBody& rb = physics.bodies.back();
    rb.velocity = vel;
    rb.orientation = ori;

    rb.inv_inertia_world = inv_inertia_world_from_body(rb.orientation, rb.inv_inertia_body);

    Entity& o = world.entities().back();
    o.transform.orientation = rb.orientation;
    o.color = color;
}

void EngineContext::create_pyramid(int base_n, f32 step_size, f32 base_z)
{
    for (int layer{0}; layer < base_n; ++layer)
    {
        const auto n = int{base_n - layer};
        const auto z = base_z + static_cast<f32>(layer) * step_size;

        const auto half_span = 0.5f * static_cast<f32>(n - 1) * step_size;

        for (int ix{0}; ix < n; ++ix)
        {
            const auto x = static_cast<f32>(ix) * step_size - half_span;
            const auto y = 0.0f;
            spawn_cube(
                Pos3{x, y, z},
                k_zero_dir,
                k_quaternion_identity,
                Color3{k_scene_object_default_color}
            );
            obj_name_map.insert_or_assign(
                world.entities().back().id, std::format("Pyramid (layer={}, idx={})", layer, ix)
            );
        }
    }
}

void EngineContext::create_pyramid_3d(int base_n, f32 step_size, f32 base_z)
{
    for (int layer{0}; layer < base_n; ++layer)
    {
        const auto n = base_n - layer;
        const auto z = base_z + static_cast<f32>(layer) * step_size;

        const auto half_span = 0.5f * static_cast<f32>(n - 1) * step_size;

        for (int ix{0}; ix < n; ++ix)
        {
            const auto x = static_cast<f32>(ix) * step_size - half_span;

            for (int iy{0}; iy < n; ++iy)
            {
                const auto y = static_cast<f32>(iy) * step_size - half_span;

                spawn_cube(
                    Pos3{x, y, z},
                    k_zero_dir,
                    k_quaternion_identity,
                    Color3{k_scene_object_default_color}
                );

                obj_name_map.insert_or_assign(
                    world.entities().back().id,
                    std::format("Pyramid3D (layer={}, ix={}, iy={})", layer, ix, iy)
                );
            }
        }
    }
}

bool EngineContext::setup()
{
    setup_active_scene(*this);
    gfx.engine_context = this;
    if (!gfx.setup())
    {
        return false;
    }

    frame_time = Clock::now();
    physics.time = frame_time;
    accumulator = Duration{0.0};

    return true;
}

void EngineContext::run()
{
    const Duration fixed_dt{physics.time_step};
    const Duration max_frame_dt{0.25};

    frame_time = Clock::now();
    physics.time = frame_time;
    accumulator = Duration{0.0};

    bool prev_paused{paused};
    while (gfx.is_active())
    {
        gfx.step();
        if (!gfx.imgui_uses_keyboard && gfx.editor_input.key_pressed(EditorKey::Space))
        {

            paused = !paused;
        }

        if (paused != prev_paused)
        {
            if (paused)
            {
                ui_log("Paused (SPACE to resume)");
            }
            else
            {
                ui_log("Running (SPACE to pause)");
                accumulator = Duration{0.0};
                frame_time = Clock::now();
                physics.time = frame_time;
            }
            prev_paused = paused;
        }

        const TimePoint now{Clock::now()};
        Duration frame_dt{std::chrono::duration_cast<Duration>(now - frame_time)};
        frame_time = now;
        frame_dt = std::min(frame_dt, max_frame_dt);

        if (!paused)
        {
            accumulator += frame_dt;
            while (accumulator >= fixed_dt)
            {
                physics.step();
                accumulator -= fixed_dt;
            }

            for (const auto& [id, idxs] : obj_map)
            {
                const auto [cube_i, phys_i] = idxs;
                world.entity(cube_i).transform.position = physics.bodies[phys_i].position;
                world.entity(cube_i).transform.orientation = physics.bodies[phys_i].orientation;
            }
        }
    }
}

}  // namespace ds_pba
