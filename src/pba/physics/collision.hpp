// pba/physics/collision.hpp
#pragma once

#include "pba/physics/physics_types.hpp"

#include <vector>

namespace ds_pba
{
[[nodiscard]] ContactKey
make_contact_key(const RigidBody& a, const RigidBody& b, const Position3& p) noexcept;

void generate_obb_contacts(std::span<const RigidBody> bodies, std::vector<Contact>& out);
}  // namespace ds_pba
