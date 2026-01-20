// pba/main.cpp
#include "pba/pch.hpp"
//
#include "pba/render_context.hpp"
#include "pba/scene_context.hpp"
#include "pba/shutdown.hpp"

#include <csignal>

namespace
{
extern "C" void handle_term(int) noexcept
{
    ds_pba::g_request_close.store(true, std::memory_order_relaxed);
}
}  // namespace

int main()
{
    using namespace ds_pba;

    std::signal(SIGTERM, handle_term);
    std::signal(SIGINT, handle_term);

    RenderContext render_context{};
    if (!render_context.setup())
    {
        return EXIT_FAILURE;
    }

    auto scene = std::make_unique<SceneContext>();
    scene->setup();
    render_context.scene_context = std::move(scene);

    render_context.run();
    return EXIT_SUCCESS;
}
