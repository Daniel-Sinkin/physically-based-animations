// pba/main.cpp
#include "pba/core_types.hpp"
#include "pba/engine_context.hpp"
#include "pba/math_types.hpp"
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/render_context.hpp"
#include "pba/scene_context.hpp"
#include "pba/shutdown.hpp"

namespace
{
extern "C" void handle_term(int) noexcept
{
    // Gracefully shut down when terminal sends close request;
    // this was implemented for proper watcher integration
    ds_pba::g_request_close.store(true, std::memory_order_relaxed);
}
}  // namespace

int main()
{
    using namespace ds_pba;

    std::signal(SIGTERM, handle_term);
    std::signal(SIGINT, handle_term);

    EngineContext engine{};
    if (!engine.setup())
    {
        return EXIT_FAILURE;
    }

    engine.run();
}
