// hash.hpp
#pragma once

#include "pba/core/core_types.hpp"

namespace ds_pba
{
// This is based on the boost implementation of hash_mix
// https://www.boost.org/doc/libs/1_84_0/boost/container_hash/detail/hash_mix.hpp
inline constexpr usize k_hash_combine_constant = 0x9e3779b9zu;

[[nodiscard]] inline auto hash_combine_seed(usize seed, usize v) noexcept -> usize
{
    seed ^= v + k_hash_combine_constant + (seed << 6) + (seed >> 2);
    return seed;
}
}  // namespace ds_pba
