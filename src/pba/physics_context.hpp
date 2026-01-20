// pba/engine_context.hpp
#pragma once
#include "pba/core_types.hpp"
#include "pba/math_types.hpp"

#include <print>

namespace ds_pba
{
constexpr f32 k_static_mass{0.0f};
constexpr Direction3 k_gravity{0.0f, 0.0f, -9.81f};

struct AABB
{
    Position3 min;
    Position3 max;
};

struct RigidBody
{
    ObjectId id;
    AABB collider;
    Position3 position;
    Direction3 velocity;
    f32 inv_mass;

    AABB get_world_collider()
    {
        return {.min = collider.min + position, .max = collider.max + position};
    }
};

struct PhysicsContext
{
    std::vector<RigidBody> bodies;

    void apply_forces(f32 delta_time)
    {
        // For now just gravity, no collisions
        for (RigidBody& body : bodies)
        {
            body.velocity += k_gravity * delta_time;
        }
    }

    void update_positions(f32 delta_time)
    {
        for (RigidBody& body : bodies)
        {
            body.position += body.velocity * delta_time;
            if (body.position.z <= 0.5f)
            {
                if (body.velocity.z < -0.1f)
                {
                    body.velocity.z = 0.95f * std::abs(body.velocity.z);
                }
                else
                {
                    body.velocity.z = 0.0f;
                    body.position.z = 0.5f;
                }
            }
        }
    }
};
}  // namespace ds_pba
