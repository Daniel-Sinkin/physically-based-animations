// pba/main.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "app/headless_main.hpp"
//
#include "pba/engine/engine_context.hpp"
#include "pba/util/scope_timer.hpp"
#include "pba/util/shutdown.hpp"
//
#include <cstdlib>
#include <cstring>
//
#include <gsl/assert>

namespace
{
extern "C" auto handle_term(int) noexcept -> void
{
    // See shutdown.hpp for details on signal handling
    ds_pba::g_request_close_sig = 1;
}

auto print_gui_usage(std::string_view program_name) -> void
{
    std::println("Usage:");
    std::println("  {}                  # run GUI", program_name);
    std::println("  {} --headless ...   # run headless physics", program_name);
    ds_pba::print_headless_usage(std::format("{} --headless", program_name));
}
}  // namespace

auto main(int argc, char** argv) -> int
{
    using namespace ds_pba;
    const ScopeTimer timer{"Total Runtime"};

    std::signal(SIGTERM, handle_term);
    std::signal(SIGINT, handle_term);

    if (argc > 1 && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0))
    {
        print_gui_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (argc > 1 && std::strcmp(argv[1], "--headless") == 0)
    {
        auto options = HeadlessRunOptions{};
        auto error = std::string{};

        auto args_vec = std::vector<const char*>{};
        args_vec.reserve(static_cast<usize>(argc > 2 ? argc - 2 : 0));
        for (auto i = 2; i < argc; ++i)
        {
            args_vec.push_back(argv[i]);
        }

        const auto args = std::span<const char* const>{args_vec.data(), args_vec.size()};
        if (!parse_headless_options(args, options, error))
        {
            std::println(stderr, "{}", error);
            print_headless_usage(std::format("{} --headless", argv[0]));
            return EXIT_FAILURE;
        }
        if (options.show_help)
        {
            print_headless_usage(std::format("{} --headless", argv[0]));
            return EXIT_SUCCESS;
        }
        return run_headless_simulation(options);
    }

    if (argc > 1)
    {
        std::println(stderr, "Unknown argument '{}'", argv[1]);
        print_gui_usage(argv[0]);
        return EXIT_FAILURE;
    }

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
