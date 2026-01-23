// pba/physics_context.hpp
#pragma once

#include "pba/core_types.hpp"
#include "pba/math_types.hpp"

#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace ds_pba
{

constexpr f32 k_static_mass{0.0f};
constexpr Direction3 k_gravity{0.0f, 0.0f, -9.81f};

f32 center_axis(f32 mn, f32 mx) noexcept;

struct RigidBody
{
    ObjectId id{k_invalid_id};

    // Half Extents
    Direction3 half_extents{0.5f, 0.5f, 0.5f};

    // Linear
    Position3 position{0.0f, 0.0f, 0.0f};
    Direction3 velocity{0.0f, 0.0f, 0.0f};
    Direction3 force_accum{0.0f, 0.0f, 0.0f};
    f32 inv_mass{k_static_mass};

    // Angular
    Quaternion orientation{1.0f, 0.0f, 0.0f, 0.0f};
    Direction3 angular_velocity{0.0f, 0.0f, 0.0f};  // omega (world space)
    Direction3 torque_accum{0.0f, 0.0f, 0.0f};

    glm::mat3 inv_inertia_body{0.0f};   // constant
    glm::mat3 inv_inertia_world{0.0f};  // based on orientation

    [[nodiscard]] bool is_static() const noexcept
    {
        return inv_mass == k_static_mass;
    }
};

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
