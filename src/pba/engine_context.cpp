// pba/engine_context.hpp
#include "pba/engine_context.hpp"
//
#include "pba/core_types.hpp"
#include "pba/util/scope_timer.hpp"

#include <print>

namespace ds_pba
{

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
            .collider =
                AABB{
                    .min = Position3{-0.5f, -0.5f, -0.5f},
                    .max = Position3{0.5f, 0.5f, 0.5f},
                },
            .position = position,
            .velocity = Direction3{0.0f, 0.0f, 0.0f},
            .inv_mass = 1.0f,
        }
    );

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
            .collider =
                AABB{
                    .min = -half_extents,
                    .max = +half_extents,
                },
            .position = ground_center,
            .velocity = Direction3{0.0f, 0.0f, 0.0f},
            .inv_mass = k_static_mass,
        }
    );

    link_latest_objects(id);
}

bool EngineContext::setup()
{
    const util::ScopeTimer timer{"Engine setup"};
    {
        add_ground();

        constexpr int n{1};
        constexpr f32 s{1.10f};

        for (int y{0}; y < n; ++y)
        {
            for (int x{0}; x < n; ++x)
            {
                const f32 x_f = static_cast<f32>(x);
                const f32 y_f = static_cast<f32>(y);

                const f32 px = (x_f - (n - 1) * 0.5f) * s;
                const f32 py = (y_f - (n - 1) * 0.5f) * s;
                const f32 pz = 5.0f + s * (x_f + y_f);

                add_cube({px, py, pz});
            }
        }
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

        if (frame_dt > max_frame_dt)
        {  // Clamp so we don't get stuck on breakpoints and computer hiccups
            frame_dt = max_frame_dt;
        }

        int n_phys_updates{0};
        accumulator += frame_dt;
        while (accumulator >= fixed_dt)
        {
            physics->step();
            accumulator -= fixed_dt;
            ++n_phys_updates;
        }
        std::println("Did {} physics updates for frame #{}", n_phys_updates, renderer->frame_count);

        for (const auto& [id, idxs] : obj_map)
        {
            const auto [scene_i, phys_i] = idxs;
            scene->cube_objects[scene_i].transform.position = physics->bodies[phys_i].position;
        }

        renderer->step();
    }
}

}  // namespace ds_pba
