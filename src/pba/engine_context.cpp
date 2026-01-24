// pba/engine_context.cpp
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine_context.hpp"
//
#include "pba/format.hpp"  // IWYU pragma: keep
#include "pba/util/scope_timer.hpp"

#include <algorithm>

namespace ds_pba
{
namespace
{

[[nodiscard]] glm::mat3 inv_inertia_body_box(f32 inv_mass, const Direction3& he) noexcept
{
    if (inv_mass == k_static_mass)
    {
        return glm::mat3(0.0f);
    }

    const f32 m = 1.0f / inv_mass;

    const f32 x = 2.0f * he.x;
    const f32 y = 2.0f * he.y;
    const f32 z = 2.0f * he.z;

    const f32 Ixx = (m / 12.0f) * (y * y + z * z);
    const f32 Iyy = (m / 12.0f) * (x * x + z * z);
    const f32 Izz = (m / 12.0f) * (x * x + y * y);

    glm::mat3 invI(0.0f);
    invI[0][0] = (Ixx > 0.0f) ? (1.0f / Ixx) : 0.0f;
    invI[1][1] = (Iyy > 0.0f) ? (1.0f / Iyy) : 0.0f;
    invI[2][2] = (Izz > 0.0f) ? (1.0f / Izz) : 0.0f;
    return invI;
}

[[nodiscard]] glm::mat3
inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
{
    const glm::mat3 R = glm::mat3_cast(q);
    return R * inv_inertia_body * glm::transpose(R);
}

void init_box_inertia(RigidBody& b) noexcept
{
    b.inv_inertia_body = inv_inertia_body_box(b.inv_mass, b.half_extents);
    b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
}

}  // namespace

EngineContext::EngineContext()
    : scene(std::make_unique<SceneContext>()), renderer(std::make_unique<RenderContext>()),
      physics(std::make_unique<PhysicsContext>())
{
}

void EngineContext::link_latest_objects(ObjectId id)
{
    obj_map.insert_or_assign(
        id,
        ObjectLink{
            scene->cube_objects.size() - 1zu,
            physics->bodies.size() - 1zu,
        }
    );
}

void EngineContext::add_cube(Position3 position)
{
    const ObjectId id = next_object_id();

    scene->cube_objects.push_back(
        Object{.id = id, .type = ObjectType::Cube, .transform = {.position = position}}
    );

    physics->bodies.push_back(
        RigidBody{
            .id = id,

            .half_extents = Direction3{0.5f, 0.5f, 0.5f},

            .position = position,
            .velocity = Direction3{0.0f, 0.0f, 0.0f},
            .force_accum = Direction3{0.0f, 0.0f, 0.0f},
            .inv_mass = 1.0f,

            .orientation = Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
            .angular_velocity = Direction3{0.0f, 0.0f, 0.0f},
            .torque_accum = Direction3{0.0f, 0.0f, 0.0f},

            .inv_inertia_body = glm::mat3(0.0f),
            .inv_inertia_world = glm::mat3(0.0f),
        }
    );
    RigidBody& b{physics->bodies.back()};
    const Quaternion qx = glm::angleAxis(glm::radians(45.0f), Direction3{1.0f, 0.0f, 0.0f});
    const Quaternion qy = glm::angleAxis(glm::radians(45.0f), Direction3{0.0f, 1.0f, 0.0f});
    b.orientation = glm::normalize(qy * qx);

    init_box_inertia(physics->bodies.back());

    link_latest_objects(id);
}

void EngineContext::add_ground()
{
    const ObjectId id = next_object_id();

    constexpr Position3 ground_center{0.0f, 0.0f, -0.5f};
    constexpr Position3 half_extents{10.0f, 10.0f, 0.5f};

    scene->cube_objects.push_back(
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

    physics->bodies.push_back(
        RigidBody{
            .id = id,

            .half_extents = half_extents,

            .position = ground_center,
            .velocity = Direction3{0.0f, 0.0f, 0.0f},
            .force_accum = Direction3{0.0f, 0.0f, 0.0f},
            .inv_mass = k_static_mass,

            .orientation = Quaternion{1.0f, 0.0f, 0.0f, 0.0f},
            .angular_velocity = Direction3{0.0f, 0.0f, 0.0f},
            .torque_accum = Direction3{0.0f, 0.0f, 0.0f},

            .inv_inertia_body = glm::mat3(0.0f),
            .inv_inertia_world = glm::mat3(0.0f),
        }
    );

    init_box_inertia(physics->bodies.back());

    link_latest_objects(id);
}

bool EngineContext::setup()
{
    const util::ScopeTimer timer{"Engine setup"};
    {
        add_ground();

        add_cube(Position3{-2.0f, 0.0f, 1.0f});
        physics->bodies.back().velocity = Direction3{+3.0f, 0.0f, 0.0f};

        add_cube(Position3{+2.0f, 0.0f, 1.0f});
        physics->bodies.back().velocity = Direction3{-3.0f, 0.0f, 0.0f};
    }

    renderer->scene_context = scene.get();
    renderer->engine_context = this;
    if (!renderer->setup())
    {
        return false;
    }

    frame_time = Clock::now();
    physics->time = frame_time;
    accumulator = Duration{0.0};

    return true;
}

void EngineContext::run()
{
    const Duration fixed_dt = physics->time_step;
    const Duration max_frame_dt{0.25};

    frame_time = Clock::now();
    physics->time = frame_time;
    accumulator = Duration{0.0};

    while (renderer->is_active())
    {
        const TimePoint now = Clock::now();
        Duration frame_dt = std::chrono::duration_cast<Duration>(now - frame_time);
        frame_time = now;

        // Clamp so we don't get stuck on breakpoints and computer hiccups
        frame_dt = std::min(frame_dt, max_frame_dt);

        [[maybe_unused]] int n_phys_updates{0};
        accumulator += frame_dt;
        while (accumulator >= fixed_dt)
        {
            physics->step();
            accumulator -= fixed_dt;
            ++n_phys_updates;
        }

        for (const auto& [id, idxs] : obj_map)
        {
            const auto [scene_i, phys_i] = idxs;
            scene->cube_objects[scene_i].transform.position = physics->bodies[phys_i].position;
            scene->cube_objects[scene_i].transform.orientation =
                physics->bodies[phys_i].orientation;
        }

        renderer->step();
    }
}

}  // namespace ds_pba
