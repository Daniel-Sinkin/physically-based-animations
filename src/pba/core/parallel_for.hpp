// pba/core/parallel_for.hpp
#pragma once

#include "pba/core/core_types.hpp"

namespace ds_pba
{

template <class Fn>
inline auto parallel_for_index(usize count, Fn&& fn) -> void
{ // Got the idea for this from a talk discussing the parallel execution policy (which in
  // turn was inspired by the CUDA Thrust library
  // https://en.cppreference.com/w/cpp/algorithm/execution_policy_tag_t.html
#if defined(DS_PBA_USE_OPENMP)
#    pragma omp parallel for schedule(static)
    for (i64 i = 0; i < static_cast<i64>(count); ++i)
    { // It's more portable for OMP to use long long (== i64) 
        fn(static_cast<usize>(i));
    }
#else
    for (auto i = 0zu; i < count; ++i)
    {
        fn(i);
    }
#endif
}

}  // namespace ds_pba

