// app/headless_entry.cpp
#include "app/headless_main.hpp"

#include <cstdlib>
#include <print>
#include <span>
#include <vector>

auto main(int argc, char** argv) -> int
{
    using namespace ds_pba;

    auto options = HeadlessRunOptions{};
    auto error = std::string{};

    auto args_vec = std::vector<const char*>{};
    args_vec.reserve(static_cast<usize>(argc > 1 ? argc - 1 : 0));
    for (auto i = 1; i < argc; ++i)
    {
        args_vec.push_back(argv[i]);
    }

    const auto args = std::span<const char* const>{args_vec.data(), args_vec.size()};
    if (!parse_headless_options(args, options, error))
    {
        std::println(stderr, "{}", error);
        print_headless_usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (options.show_help)
    {
        print_headless_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    return run_headless_simulation(options);
}
