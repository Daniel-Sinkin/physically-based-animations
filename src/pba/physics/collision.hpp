// pba/physics/collision.hpp
#pragma once

#include "pba/core/arena_allocator.hpp"
#include "pba/physics/physics_types.hpp"

#include <vector>

namespace ds_pba
{
[[nodiscard]] auto make_contact_key(const RigidBody& a, const RigidBody& b, const Pos3& p) noexcept
    -> ContactKey;

auto generate_obb_contacts(std::span<const RigidBody> bodies, ArenaAllocator& out) -> void;
}  // namespace ds_pba
