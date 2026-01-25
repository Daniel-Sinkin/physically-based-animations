// pba/engine/engine_context.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine/engine_context.hpp"
//
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/constants.hpp"
#include "pba/ui/ui.hpp"
#include "pba/util/scope_timer.hpp"

#include <algorithm>

namespace ds_pba
{
namespace
{

[[nodiscard]] glm::mat3 inv_inertia_body_box(f32 inv_mass, const Direction3& half_extent) noexcept
{
    if (inv_mass == k_static_mass)
    {
        return glm::mat3(0.0f);
    }

    const f32 m{1.0f / inv_mass};

    const f32 x{2.0f * half_extent.x};
    const f32 y{2.0f * half_extent.y};
    const f32 z{2.0f * half_extent.z};

    const f32 Ixx{(m / 12.0f) * (y * y + z * z)};
    const f32 Iyy{(m / 12.0f) * (x * x + z * z)};
    const f32 Izz{(m / 12.0f) * (x * x + y * y)};

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

void init_box_inertia(RigidBody& b) noexcept
{
    b.inv_inertia_body = inv_inertia_body_box(b.inv_mass, b.half_extents);
    b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
}

}  // namespace

void EngineContext::link_latest_objects(ObjectId id)
{
    obj_map.insert_or_assign(
        id,
        ObjectLink{
            scene.cube_objects.size() - 1zu,
            physics.bodies.size() - 1zu,
        }
    );
}

void EngineContext::add_cube(Position3 position)
{
    const ObjectId id{next_object_id()};

    scene.cube_objects.push_back(
        Object{.id = id, .type = ObjectType::Cube, .transform = {.position = position}}
    );

    physics.bodies.push_back(
        RigidBody{
            .id = id,

            .half_extents = Direction3{0.5f, 0.5f, 0.5f},

            .position = position,
            .velocity = Direction3{},
            .force_accum = Direction3{},
            .inv_mass = 1.0f,

            .orientation = Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
            .angular_velocity = Direction3{},
            .torque_accum = Direction3{},

            .inv_inertia_body = glm::mat3{0.0f},
            .inv_inertia_world = glm::mat3{0.0f},
        }
    );
    init_box_inertia(physics.bodies.back());
    link_latest_objects(id);
}

void EngineContext::add_ground()
{
    const ObjectId id{next_object_id()};

    constexpr Position3 ground_center{0.0f, 0.0f, -0.5f};
    constexpr Position3 half_extents{10.0f, 10.0f, 0.5f};

    scene.cube_objects.push_back(
        Object{
            .id = id,
            .type = ObjectType::Cube,
            .transform =
                {
                    .position = ground_center,
                    .scale = half_extents * 2.0f,
                },
            .color = {0.1f, 0.1f, 0.1f},
        }
    );

    physics.bodies.push_back(
        RigidBody{
            .id = id,

            .half_extents = half_extents,

            .position = ground_center,
            .velocity = Direction3{},
            .force_accum = Direction3{},
            .inv_mass = k_static_mass,

            .orientation = Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
            .angular_velocity = Direction3{},
            .torque_accum = Direction3{},

            .inv_inertia_body = glm::mat3(0.0f),
            .inv_inertia_world = glm::mat3(0.0f),
        }
    );

    init_box_inertia(physics.bodies.back());

    link_latest_objects(id);
    obj_name_map.insert_or_assign(id, "Ground");
}

bool EngineContext::setup()
{
    const util::ScopeTimer timer{"Engine setup"};
    {
        add_ground();

        auto spawn_cube = [&](Position3 pos,
                              Direction3 vel = Direction3{0.0f, 0.0f, 0.0f},
                              Quaternion ori = Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
                              ColorRGBf color = ColorRGBf{0.80f, 0.80f, 0.80f}) -> void
        {
            add_cube(pos);

            RigidBody& rb = physics.bodies.back();
            rb.velocity = vel;
            rb.orientation = ori;
            rb.angular_velocity = Direction3{0.0f, 0.0f, 0.0f};

            rb.inv_inertia_world = inv_inertia_world_from_body(rb.orientation, rb.inv_inertia_body);

            Object& o = scene.cube_objects.back();
            o.transform.orientation = rb.orientation;
            o.color = color;
        };
        spawn_cube(
            Position3{-14.0f, 0.0f, 2.0f},
            Direction3{+28.0f, 0.0f, 0.0f},
            Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
            ColorRGBf{0.90f, 0.35f, 0.35f}
        );
        obj_name_map.insert_or_assign(scene.cube_objects.back().id, "Projecticle Red");
        spawn_cube(
            Position3{+14.0f, 0.0f, 2.2f},
            Direction3{-28.0f, 0.0f, 0.0f},
            Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
            ColorRGBf{0.35f, 0.90f, 0.35f}
        );
        obj_name_map.insert_or_assign(scene.cube_objects.back().id, "Projecticle Green");
        spawn_cube(
            Position3{0.0f, -14.0f, 2.0f},
            Direction3{0.0f, +28.0f, 0.0f},
            Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
            ColorRGBf{0.35f, 0.55f, 0.95f}
        );
        obj_name_map.insert_or_assign(scene.cube_objects.back().id, "Projecticle Blue");
        spawn_cube(
            Position3{0.0f, +14.0f, 2.4f},
            Direction3{0.0f, -28.0f, -2.0f},
            Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
            ColorRGBf{0.95f, 0.85f, 0.35f}
        );
        obj_name_map.insert_or_assign(scene.cube_objects.back().id, "Projecticle Yellow");

        constexpr f32 cube{1.0f};
        constexpr f32 gap{0.06f};
        constexpr f32 step{cube + gap};

        constexpr int base_n{5};
        constexpr f32 base_z{0.5f};

        for (int layer{0}; layer < base_n; ++layer)
        {
            const int n{base_n - layer};
            const f32 z{base_z + static_cast<f32>(layer) * step};

            const f32 half_span{0.5f * static_cast<f32>(n - 1) * step};

            for (int ix{0}; ix < n; ++ix)
            {
                const f32 x{static_cast<f32>(ix) * step - half_span};
                const f32 y{};

                spawn_cube(
                    Position3{x, y, z},
                    Direction3{},
                    Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
                    ColorRGBf{k_scene_object_default_color}
                );
                obj_name_map.insert_or_assign(
                    scene.cube_objects.back().id,
                    std::format("Pyramid (layer={}, idx={})", layer, ix)
                );
            }
        }
    }

    gfx.scene_context = &scene;
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

    while (gfx.is_active())
    {
        const TimePoint now{Clock::now()};

        Duration frame_dt{std::chrono::duration_cast<Duration>(now - frame_time)};
        frame_time = now;

        frame_dt = std::min(frame_dt, max_frame_dt);

        const bool space_down{glfwGetKey(gfx.window, GLFW_KEY_SPACE) == GLFW_PRESS};
        if (space_down && !prev_space)
        {
            paused = !paused;

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
        }
        prev_space = space_down;

        if (!paused)
        {
            [[maybe_unused]] int n_phys_updates{0};
            accumulator += frame_dt;

            while (accumulator >= fixed_dt)
            {
                physics.step();
                accumulator -= fixed_dt;
                ++n_phys_updates;
            }
        }
        for (const auto& [id, idxs] : obj_map)
        {
            const auto [scene_i, phys_i] = idxs;
            scene.cube_objects[scene_i].transform.position = physics.bodies[phys_i].position;
            scene.cube_objects[scene_i].transform.orientation = physics.bodies[phys_i].orientation;
        }

        gfx.step();
    }
}

}  // namespace ds_pba
