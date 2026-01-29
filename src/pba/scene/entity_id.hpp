// pba/scene/entity_id.hpp
#include "pba/core/core_types.hpp"

#pragma once

namespace ds_pba
{
using EntityId = u32;
inline constexpr EntityId k_invalid_id{std::numeric_limits<EntityId>::max()};
}  // namespace ds_pba
