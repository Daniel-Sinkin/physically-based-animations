// pba/physics_context.hpp
#pragma once

#include "pba/core_types.hpp"
#include "pba/math_types.hpp"

#include <vector>

namespace ds_pba
{

constexpr f32 k_static_mass{0.0f};
constexpr Direction3 k_gravity{0.0f, 0.0f, -9.81f};

struct AABB
{
    Position3 min;
    Position3 max;
};

struct Contact
{
    bool hit{false};
    Direction3 normal{0.0f, 0.0f, 0.0f};
    f32 penetration{0.0f};
};

f32 center_axis(f32 mn, f32 mx) noexcept;
Contact aabb_contact(const AABB& a, const AABB& b) noexcept;

struct RigidBody
{
    ObjectId id;
    AABB collider;
    Position3 position;
    Direction3 velocity;
    f32 inv_mass;

    AABB get_world_collider() const;
    bool is_static() const noexcept;
};

void resolve_contact(RigidBody& a, RigidBody& b, const Contact& c, f32 restitution);

struct PhysicsContext
{
    std::vector<RigidBody> bodies;

    TimePoint time{};
    Duration time_step{std::chrono::duration<f64>(1.0 / 120.0)};

    void apply_forces();
    void update_positions();
    void solve_collisions();
    void step();
};

}  // namespace ds_pba
