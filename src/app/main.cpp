// pba/main.cpp
#include "pba/pch.hpp"
//
#include "pba/render_context.hpp"
#include "pba/scene_context.hpp"

#include <print>

int main()
{
    using namespace ds_pba;

    RenderContext render_context{};
    if (!render_context.setup())
    {
        return EXIT_FAILURE;
    }

    auto scene = std::make_unique<SceneContext>();
    scene->setup();
    render_context.scene_context = std::move(scene);

    render_context.run();
}
