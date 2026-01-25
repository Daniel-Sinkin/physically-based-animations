// pba/main.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/engine/engine_context.hpp"
#include "pba/util/scope_timer.hpp"
#include "pba/util/shutdown.hpp"

#include <cstdlib>

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
