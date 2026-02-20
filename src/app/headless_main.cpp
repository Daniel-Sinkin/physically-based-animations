// app/headless_main.cpp
#include "app/headless_main.hpp"

#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/physics/physics_types.hpp"
#include "pba/simulation/scenes.hpp"
#include "pba/simulation/simulation_context.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <print>

namespace ds_pba
{
namespace
{

[[nodiscard]] auto parse_usize(std::string_view text, usize& out) -> bool
{
    auto parsed = u64{0u};
    const auto begin = text.data();
    const auto end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed);
    if (ec != std::errc{} || ptr != end)
    {
        return false;
    }
    if (parsed > std::numeric_limits<usize>::max())
    {
        return false;
    }
    out = static_cast<usize>(parsed);
    return true;
}

[[nodiscard]] auto parse_scene_id(std::string_view text, SceneId& out) -> bool
{
    auto idx = usize{0zu};
    if (!parse_usize(text, idx))
    {
        return false;
    }
    const auto scene = scene_id_from_index(idx);
    if (!scene.has_value())
    {
        return false;
    }
    out = *scene;
    return true;
}

}  // namespace

auto print_headless_usage(std::string_view program_name) -> void
{
    const auto default_idx = scene_index(k_default_scene).value_or(0zu);

    std::println("Headless usage:");
    std::println("  {} [options]", program_name);
    std::println("Options:");
    std::println("  --steps <N>       Number of physics steps (default: 2000)");
    std::println(
        "  --scene <ID>      Scene index [0..{}] (default: {})",
        scene_count() - 1zu,
        default_idx
    );
    std::println("  --progress        Print periodic progress");
    std::println("  --track-debug     Keep debug tracking enabled");
    std::println("  --help, -h        Show this help");
    std::println("Scenes:");
    for (usize i{0zu}; i < scene_count(); ++i)
    {
        const auto id = scene_id_from_index(i);
        if (!id.has_value())
        {
            continue;
        }
        std::println("  {:>2}  {}", i, scene_name(*id));
    }
}

auto parse_headless_options(
    std::span<const char* const> args, HeadlessRunOptions& options, std::string& error
) -> bool
{
    for (auto i = 0zu; i < args.size(); ++i)
    {
        const auto arg = std::string_view{args[i]};

        if (arg == "--help" || arg == "-h")
        {
            options.show_help = true;
            continue;
        }
        if (arg == "--progress")
        {
            options.print_progress = true;
            continue;
        }
        if (arg == "--track-debug")
        {
            options.debug_tracking = true;
            continue;
        }
        if (arg == "--steps")
        {
            if ((i + 1zu) >= args.size())
            {
                error = "Missing value after --steps";
                return false;
            }
            auto parsed_steps = usize{0zu};
            if (!parse_usize(args[i + 1zu], parsed_steps) || parsed_steps == 0zu)
            {
                error = std::format("Invalid --steps value '{}'", args[i + 1zu]);
                return false;
            }
            options.steps = parsed_steps;
            ++i;
            continue;
        }
        if (arg == "--scene")
        {
            if ((i + 1zu) >= args.size())
            {
                error = "Missing value after --scene";
                return false;
            }
            auto scene = SceneId{};
            if (!parse_scene_id(args[i + 1zu], scene))
            {
                error = std::format(
                    "Invalid --scene value '{}' (expected [0..{}])",
                    args[i + 1zu],
                    scene_count() - 1zu
                );
                return false;
            }
            options.scene = scene;
            ++i;
            continue;
        }

        error = std::format("Unknown option '{}'", arg);
        return false;
    }
    return true;
}

auto run_headless_simulation(const HeadlessRunOptions& options) -> int
{
    using std::chrono::duration_cast;

    auto simulation = SimulationContext{};
    simulation.active_scene = options.scene;
    setup_active_scene(simulation);
    simulation.physics.set_debug_tracking_enabled(options.debug_tracking);

    auto sim_time = Duration{0.0};
    simulation.physics.time = TimePoint{};
    const auto fixed_dt = Duration{simulation.physics.time_step};

    auto total_step_ms = 0.0;
    auto max_step_ms = 0.0;
    const auto wall_start = Clock::now();

    for (auto step = 0zu; step < options.steps; ++step)
    {
        const auto t0 = Clock::now();
        simulation.physics.step();
        const auto t1 = Clock::now();

        const auto step_ms = std::chrono::duration<f64, std::milli>(t1 - t0).count();
        total_step_ms += step_ms;
        max_step_ms = std::max(max_step_ms, step_ms);

        sim_time += fixed_dt;
        simulation.physics.time += duration_cast<Clock::duration>(fixed_dt);

        if (options.print_progress
            && (((step + 1zu) % 120zu == 0zu) || ((step + 1zu) == options.steps)))
        {
            const auto contacts_last = simulation.physics.contact_arena.used() / sizeof(Contact);
            std::println(
                "Headless step {}/{} | t={:.3f}s | contacts(last)={}",
                step + 1zu,
                options.steps,
                sim_time.count(),
                contacts_last
            );
        }
    }

    const auto wall_total_s = std::chrono::duration<f64>(Clock::now() - wall_start).count();
    const auto avg_step_ms = (options.steps > 0zu) ? (total_step_ms / static_cast<f64>(options.steps))
                                                   : 0.0;
    const auto sim_hz = (avg_step_ms > 1e-9) ? (1000.0 / avg_step_ms) : 0.0;
    const auto contacts_last = simulation.physics.contact_arena.used() / sizeof(Contact);

    std::println("Headless run complete:");
    std::println(
        "  scene: {} ({})",
        scene_index(options.scene).value_or(0zu),
        scene_name(options.scene)
    );
    std::println("  steps: {}", options.steps);
    std::println("  bodies: {}", simulation.physics.body_count());
    std::println("  wall time: {:.3f} s", wall_total_s);
    std::println("  avg step: {:.3f} ms ({:.1f} Hz)", avg_step_ms, sim_hz);
    std::println("  max step: {:.3f} ms", max_step_ms);
    std::println("  contacts last step: {}", contacts_last);
    std::println("  debug tracking: {}", options.debug_tracking ? "on" : "off");

    return EXIT_SUCCESS;
}

}  // namespace ds_pba
