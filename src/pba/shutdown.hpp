// pba/shutdown.hpp
#pragma once

#include <atomic>

namespace ds_pba
{
inline std::atomic_bool g_request_close{false};
}  // namespace ds_pba
