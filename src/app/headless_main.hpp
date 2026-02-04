// app/headless_main.cpp
//
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/simulation/scenes.hpp"
#include "pba/simulation/simulation_context.hpp"
#include "pba/util/scope_timer.hpp"

#include <chrono>
#include <print>

// TOOD: Make this into a .cpp and compile seperately

auto run_headless_simulation() -> void
{
    using namespace ds_pba;
    using std::chrono::duration_cast;

    ScopeTimer timer{"Headless Simulation"};

    SimulationContext simulation{};
    setup_active_scene(simulation);

    Duration sim_time{0.0};
    simulation.physics.time = TimePoint{};

    const Duration fixed_dt{simulation.physics.time_step};

    while (true)
    {
        std::println(
            "Running Physics Simulation, t = {:.6f} s", static_cast<f64>(sim_time.count())
        );

        {
            ScopeTimer timer_inner{"Single Step Duration"};
            simulation.physics.step();
        }

        sim_time += fixed_dt;
        simulation.physics.time += duration_cast<Clock::duration>(fixed_dt);
    }
}
