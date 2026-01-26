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
    const TimePoint end{Clock::now()};
    const auto dt = end - start_;

    const double seconds{std::chrono::duration<double>(dt).count()};
    if (seconds >= 2.0)
    {
        std::println("{}: {:.2f} s", label_, seconds);
    }
    else
    {
        const double ms{seconds * 1000.0};
        std::println("{}: {:.3f} ms", label_, ms);
    }
}

}  // namespace ds_pba
