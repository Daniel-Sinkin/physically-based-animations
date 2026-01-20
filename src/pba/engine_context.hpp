// pba/engine_context.hpp
#pragma once
#include "pba/core_types.hpp"
#include "pba/render_context.hpp"
#include "pba/scene_context.hpp"

namespace ds_pba
{
struct EngineContext
{
    std::unique_ptr<SceneContext> scene = std::make_unique<SceneContext>();
    std::unique_ptr<RenderContext> renderer = std::make_unique<RenderContext>();

    bool setup()
    {
        ObjectId id = next_object_id();
        scene->cube_objects.push_back(
            Object{
                .id = id, .type = ObjectType::Cube, .transform = {.position = {0.0f, 0.0f, 1.0f}}
            }
        );
        renderer->scene_context = scene.get();
        return renderer->setup();
    }

    void run()
    {
        renderer->run();
    }
};
}  // namespace ds_pba
