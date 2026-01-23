// pba/main.cpp
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine_context.hpp"
#include "pba/util/scope_timer.hpp"
#include "pba/util/shutdown.hpp"

#include <cstdlib>

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
    const ds_pba::util::ScopeTimer timer{"Total Runtime"};
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
