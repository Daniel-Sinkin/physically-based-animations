// pba/util/scope_timer.cpp
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/util/scope_timer.hpp"
//

namespace ds_pba::util
{

ScopeTimer::ScopeTimer(std::string_view label) noexcept
    : label_(label)
    , start_(Clock::now())
{
}

ScopeTimer::~ScopeTimer() noexcept
{
    const auto end = Clock::now();
    const auto dt = end - start_;
    const double ms = std::chrono::duration<double, std::milli>(dt).count();
    std::println("{}: {:.3f} ms", label_, ms);
}

}  // namespace ds_pba::util