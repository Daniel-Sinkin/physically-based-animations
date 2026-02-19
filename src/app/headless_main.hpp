// app/headless_main.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/simulation/scene_id.hpp"

#include <span>
#include <string>
#include <string_view>

namespace ds_pba
{

struct HeadlessRunOptions
{
    usize steps{2000zu};
    SceneId scene{k_default_scene};
    bool show_help{false};
    bool print_progress{false};
    bool debug_tracking{false};
};

auto print_headless_usage(std::string_view program_name) -> void;

[[nodiscard]] auto parse_headless_options(
    std::span<const char* const> args, HeadlessRunOptions& options, std::string& error
) -> bool;

auto run_headless_simulation(const HeadlessRunOptions& options) -> int;

}  // namespace ds_pba

