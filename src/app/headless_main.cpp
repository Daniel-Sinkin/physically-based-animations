// app/headless_main.cpp
#include "pba/engine/engine_context.hpp"
#include "pba/engine/scenes.hpp"
#include "pba/util/scope_timer.hpp"

auto run_headless_simulation() -> void
{
    using namespace ds_pba;
    ds_pba::ScopeTimer timer{"Headless Simulation"};

    EngineContext e{};
    setup_active_scene(e.simulation);
}
