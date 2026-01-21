// pba_util/scope_timer.hpp
#pragma once

#include "pba/core_types.hpp"

#include <string_view>

namespace ds_pba::util
{

class ScopeTimer
{
  public:
    explicit ScopeTimer(std::string_view label) noexcept;
    ~ScopeTimer() noexcept;

    ScopeTimer(const ScopeTimer&) = delete;
    ScopeTimer& operator=(const ScopeTimer&) = delete;

    ScopeTimer(ScopeTimer&&) = delete;
    ScopeTimer& operator=(ScopeTimer&&) = delete;

  private:
    std::string_view label_;
    Clock::time_point start_;
};

}  // namespace ds_pba::util