// pba/main.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "app/headless_main.hpp"
//
#include "pba/engine/engine_context.hpp"
#include "pba/util/scope_timer.hpp"
#include "pba/util/shutdown.hpp"
//
#include <cstddef>
#include <cstdlib>
#include <cstring>
//
#include <gsl/assert>

namespace
{
extern "C" void handle_term(int) noexcept
{
    // See shutdown.hpp for details on signal handling
    ds_pba::g_request_close_sig = 1;
}
}  // namespace

int main()
{
    using namespace ds_pba;
    const ScopeTimer timer{"Total Runtime"};

    std::signal(SIGTERM, handle_term);
    std::signal(SIGINT, handle_term);

    // run_headless_simulation();

    if constexpr (true)
    {
        EngineContext engine{};
        if (!engine.setup())
        {
            return EXIT_FAILURE;
        }

        engine.run();
    }
}
