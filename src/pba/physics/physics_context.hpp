// pba/physics/physics_context.hpp
#pragma once

#include "pba/physics/physics_types.hpp"

namespace ds_pba
{

struct PhysicsContext
{
    std::vector<RigidBody> bodies;

    TimePoint time{};
    Duration time_step{std::chrono::duration<f64>(1.0 / 120.0)};

    std::unordered_map<ContactKey, ContactCacheEntry, ContactKeyHash> contact_cache{};
    void step();
};

}  // namespace ds_pba
