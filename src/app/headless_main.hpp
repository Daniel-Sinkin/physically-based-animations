// app/headless_main.cpp
#include "pba/simulation/scenes.hpp"
#include "pba/simulation/simulation_context.hpp"
#include "pba/util/scope_timer.hpp"

// TOOD: Make this into a .cpp and compile seperately

auto run_headless_simulation() -> void
{
    using namespace ds_pba;
    ds_pba::ScopeTimer timer{"Headless Simulation"};

    SimulationContext sim{};
    setup_active_scene(sim);
}
