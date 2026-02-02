// pba/engine/engine_context.cpp
#include "glm/fwd.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine/engine_context.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/engine/scenes.hpp"
#include "pba/physics/physics_types.hpp"
#include "pba/ui/ui.hpp"
#include "pba/util/shutdown.hpp"
//
#include <algorithm>
#include <glm/gtc/quaternion.hpp>
#include <gsl/assert>

namespace ds_pba
{
namespace
{

[[nodiscard]] auto inv_inertia_body_box(f32 inv_mass, const Dir3& half_extents) noexcept
    -> glm::mat3
{
    if (inv_mass == k_static_mass)
    {
        return glm::mat3(0.0f);
    }

    Expects(inv_mass > 0.0f);
    if (inv_mass <= 0.0f)
    {
        return glm::mat3(0.0f);
    }
    const auto m = 1.0f / inv_mass;

    const auto x = 2.0f * half_extents.x;
    const auto y = 2.0f * half_extents.y;
    const auto z = 2.0f * half_extents.z;

    const auto Ixx = (m / 12.0f) * (y * y + z * z);
    const auto Iyy = (m / 12.0f) * (x * x + z * z);
    const auto Izz = (m / 12.0f) * (x * x + y * y);

    glm::mat3 invI{0.0f};
    invI[0][0] = (Ixx > 0.0f) ? (1.0f / Ixx) : 0.0f;
    invI[1][1] = (Iyy > 0.0f) ? (1.0f / Iyy) : 0.0f;
    invI[2][2] = (Izz > 0.0f) ? (1.0f / Izz) : 0.0f;
    return invI;
}

[[nodiscard]] auto
inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
    -> glm::mat3
{
    const glm::mat3 R{glm::mat3_cast(q)};
    return R * inv_inertia_body * glm::transpose(R);
}

auto init_box_inertia(RigidBody& b) noexcept -> void
{
    b.inv_inertia_body = inv_inertia_body_box(b.inv_mass, b.half_extents);
    b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
}

}  // namespace

auto EngineContext::create_box_body(const Entity& e, f32 inv_mass, Dir3 velo) const -> RigidBody
{
    RigidBody rb{
        .id = e.id,
        .half_extents = e.transform.scale * 0.5f,
        .position = e.transform.position,
        .velocity = velo,
        .inv_mass = inv_mass,
        .orientation = e.transform.orientation
    };
    init_box_inertia(rb);
    return rb;
}

auto EngineContext::spawn_cube(
    Pos3 pos,
    Dir3 half_extents,
    f32 inv_mass,
    Dir3 vel,
    Quaternion ori,
    Dir3 ang_vel,
    Color3 color,
    std::string_view name
) -> Entity&
{
    Entity& e = world.spawn(
        EntityType::Cube,
        Transform{.position = pos, .scale = half_extents * 2.0f, .orientation = ori},
        color
    );

    if (!name.empty())
    {
        e.name = std::string{name};
    }

    RigidBody rb{
        .id = e.id,
        .half_extents = half_extents,
        .position = pos,
        .velocity = vel,
        .inv_mass = inv_mass,
        .orientation = ori,
        .angular_velocity = ang_vel,
    };
    init_box_inertia(rb);

    e.body = physics.add_body(std::move(rb));
    return e;
}

auto EngineContext::add_ground() -> Entity&
{
    return spawn_cube(
        Pos3{0.0f, 0.0f, -3.5f},
        Dir3{10.0f, 10.0f, 0.5f},
        k_static_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        Color3{0.1f, 0.1f, 0.1f},
        "Ground"
    );
}

auto EngineContext::create_pyramid(int base_n, f32 step_size, f32 base_z) -> void
{
    for (int layer{0}; layer < base_n; ++layer)
    {
        const auto n = base_n - layer;
        const auto z = base_z + static_cast<f32>(layer) * step_size;
        const auto half_span = 0.5f * static_cast<f32>(n - 1) * step_size;

        for (int i{0}; i < n; ++i)
        {
            const auto x = static_cast<f32>(i) * step_size - half_span;

            auto& entity = spawn_cube(
                Pos3{x, 0.0f, z},
                Dir3{0.5f, 0.5f, 0.5f},
                1.0f,
                k_zero_dir,
                k_quaternion_identity,
                k_zero_dir,
                Color3{k_scene_object_default_color},
                {}
            );

            entity.name = std::format("Pyramid (layer={}, idx={})", layer, i);
        }
    }
}

auto EngineContext::create_pyramid_3d(int base_n, f32 step_size, f32 base_z) -> void
{
    for (int layer{0}; layer < base_n; ++layer)
    {
        const auto n = base_n - layer;
        const auto z = base_z + static_cast<f32>(layer) * step_size;
        const auto half_span = 0.5f * static_cast<f32>(n - 1) * step_size;

        for (int i{0}; i < n; ++i)
        {
            const auto x = static_cast<f32>(i) * step_size - half_span;

            for (int j{0}; j < n; ++j)
            {
                const auto y = static_cast<f32>(j) * step_size - half_span;

                auto& e = spawn_cube(
                    Pos3{x, y, z},
                    Dir3{0.5f, 0.5f, 0.5f},
                    1.0f,
                    k_zero_dir,
                    k_quaternion_identity,
                    k_zero_dir,
                    Color3{k_scene_object_default_color},
                    {}
                );

                e.name = std::format("Pyramid3D (layer={}, ix={}, iy={})", layer, i, j);
            }
        }
    }
}

auto EngineContext::setup() -> bool
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

auto EngineContext::sync_physics_to_world() -> void
{
    for (Entity& e : world.entities())
    {
        if (!e.body)
        {
            continue;
        }
        if (const RigidBody* rb = physics.try_body(*e.body))
        {
            e.transform.position = rb->position;
            e.transform.orientation = rb->orientation;
        }
    }
}

auto EngineContext::run() -> void
{
    const Duration fixed_dt{physics.time_step};
    const Duration max_frame_dt{0.25};

    frame_time = Clock::now();
    physics.time = frame_time;
    accumulator = Duration{0.0};

    bool prev_paused{paused};

    while (is_active())
    {
        {  // See shutdown.hpp for details on our signal handling
            if (g_request_close_sig)
            {
                g_request_close.store(true, std::memory_order_relaxed);
                g_request_close_sig = 0;
            }
            if (g_request_close.load(std::memory_order_relaxed))
            {
                request_close();
            }
        }

        glfwPollEvents();
        editor_input.update(not_null{gfx.window});
        gfx.step(editor_input);

        if (!gfx.imgui_uses_keyboard && editor_input.key_pressed(EditorKey::Space))
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

        if (paused)
        {
            continue;
        }

        accumulator += frame_dt;
        while (accumulator >= fixed_dt)
        {
            physics.step();
            accumulator -= fixed_dt;
        }

        sync_physics_to_world();
    }
}

}  // namespace ds_pba
