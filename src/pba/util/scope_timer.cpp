// pba/util/scope_timer.cpp
#include "pba/core/core_types.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/util/scope_timer.hpp"
//

namespace ds_pba
{

ScopeTimer::ScopeTimer(std::string_view label) noexcept : label_(label), start_(Clock::now())
{
}

ScopeTimer::~ScopeTimer() noexcept
{
    const auto dt = Clock::now() - start_;
    const auto seconds = std::chrono::duration<f64>(dt).count();
    if (seconds >= 2.0)
    {
        std::println("{}: {:.2f} s", label_, seconds);
    }
    else
    {
        std::println("{}: {:.3f} ms", label_, seconds * 1000.0);
    }
}

}  // namespace ds_pba
