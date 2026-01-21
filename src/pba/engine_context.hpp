// pba/engine_context.hpp
#pragma once
#include "pba/core_types.hpp"
#include "pba/math_types.hpp"
#include "pba/physics_context.hpp"
#include "pba/render_context.hpp"
#include "pba/scene_context.hpp"
#include "pba/scene_types.hpp"

#include <unordered_map>

namespace ds_pba
{
struct EngineContext
{
    std::unique_ptr<SceneContext> scene = std::make_unique<SceneContext>();
    std::unique_ptr<RenderContext> renderer = std::make_unique<RenderContext>();
    std::unique_ptr<PhysicsContext> physics = std::make_unique<PhysicsContext>();

    struct ObjectLink
    {
        usize cube_obj_idx;
        usize physics_obj_idx;
    };

    std::unordered_map<ObjectId, ObjectLink> obj_map{};

    void link_latest_objects(ObjectId id)
    {
        obj_map.insert_or_assign(
            id,
            ObjectLink{
                scene->cube_objects.size() - 1,
                physics->bodies.size() - 1,
            }
        );
    }

    // TODO: Add arbitrary transforms
    void add_cube(Position3 position)
    {
        ObjectId id = next_object_id();
        scene->cube_objects.push_back(
            Object{.id = id, .type = ObjectType::Cube, .transform = {.position = position}}
        );
        physics->bodies.push_back(
            RigidBody{
                .id = id,
                .collider =
                    AABB{.min = Position3{-0.5f, -0.5f, -0.5f}, .max = Position3{0.5f, 0.5f, 0.5f}},
                .position = position,
                .velocity = Direction3{0.0f, 0.0f, 0.0f},
                .inv_mass = 1.0,
            }
        );
        link_latest_objects(id);
    }

    void add_ground()
    {
        ObjectId id = next_object_id();

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

    bool setup()
    {
        {
            add_ground();

            constexpr int n = 10;
            constexpr f32 s = 1.10f;

            for (int y = 0; y < n; ++y)
            {
                for (int x = 0; x < n; ++x)
                {
                    const f32 px = (static_cast<f32>(x) - (n - 1) * 0.5f) * s;
                    const f32 py = (static_cast<f32>(y) - (n - 1) * 0.5f) * s;

                    const f32 pz = 5.0f + s * static_cast<f32>(x + y);

                    add_cube({px, py, pz});
                }
            }
        }

        {  // Setup Renderer
            renderer->scene_context = scene.get();
            renderer->physics_context = physics.get();
            renderer->engine_context = this;
            return renderer->setup();
        }
    }

    void run()
    {
        while (renderer->is_active())
        {
            // TODO: Sync renderer and physics time
            f32 physics_time = 0.0f;
            f32 physics_dt = 0.001f;
            while (physics_time < 0.008f)
            {
                physics->step(physics_dt);
                physics_time += physics_dt;
            }
            for (const auto& [id, idxs] : obj_map)
            {
                const auto [scene_i, phys_i] = idxs;
                scene->cube_objects[scene_i].transform.position = physics->bodies[phys_i].position;
            }
            renderer->step();
        }
    }
};
}  // namespace ds_pba
