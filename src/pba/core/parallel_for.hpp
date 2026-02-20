// pba/core/parallel_for.hpp
#pragma once

#include "pba/core/core_types.hpp"

namespace ds_pba
{

template <class Fn>
inline auto parallel_for_index(usize count, Fn&& fn) -> void
{
#if defined(DS_PBA_USE_OPENMP)
#    pragma omp parallel for schedule(static)
    for (i64 i = 0; i < static_cast<i64>(count); ++i)
    {
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

