// pba/main.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "app/headless_main.hpp"
#include "pba/core/arena_allocator.hpp"
#include "pba/core/core_types.hpp"
#include "pba/engine/engine_context.hpp"
#include "pba/util/scope_timer.hpp"
#include "pba/util/shutdown.hpp"
//
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <format>
#include <print>
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

namespace ds_pba
{
struct Data
{
    f32 x;
    f64 y;
    int z;
    u8 w;
};
static_assert(sizeof(Data) == 24);  // [xa xb 00 00] [ya yb yc yd] [za zb wa 00]
static_assert(alignof(Data) == sizeof(f64));

struct alignas(128) Data2
{
    f32 x;
    f64 y;
    int z;
    u8 w;
};
static_assert(sizeof(Data2) == std::max(24, 128));
static_assert(alignof(Data2) == 128);

auto to_string(const Data& data) -> std::string
{
    return std::format("(.x={},.y={},.z={},.w={})", data.x, data.y, data.z, data.w);
}

auto arena_alloc_test() -> void
{
    ArenaAllocator arena{1024};

    auto ptr = arena.push_back(Data{.x = 5.0f, .y = 2.3, .z = -1, .w = 254});
    std::println("{}", to_string(*ptr));

    ptr->x = 4.0f;
    std::println("{}", to_string(*ptr));

    std::println("{}", arena.remaining());
}
}  // namespace ds_pba

int main()
{
    using namespace ds_pba;
    const ScopeTimer timer{"Total Runtime"};

    std::signal(SIGTERM, handle_term);
    std::signal(SIGINT, handle_term);

    run_headless_simulation();

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
