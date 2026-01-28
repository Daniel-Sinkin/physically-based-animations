// pba/core/math_types.hpp
#pragma once

#include "pba/core/core_types.hpp"
//
#include <numbers>
//
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/vector_float3.hpp>

namespace ds_pba
{
using Pos3 = glm::vec3;
using Dir3 = glm::vec3;
using EulerDeg3 = glm::vec3;
using Quaternion = glm::quat;

// Common math constants
inline constexpr Dir3 k_zero_dir{0.0f, 0.0f, 0.0f};
inline constexpr Quaternion k_quaternion_identity{1.0f, 0.0f, 0.0f, 0.0f};

inline constexpr f32 k_pi{std::numbers::pi_v<f32>};
inline constexpr f32 k_two_pi{2.0f * std::numbers::pi_v<f32>};

constexpr f32 k_inf = std::numeric_limits<f32>::infinity();
constexpr f32 k_f32_max = 1e30f;
constexpr f32 k_f32_min = -1e30f;

inline constexpr Dir3 k_axis_x{1.0f, 0.0f, 0.0f};
inline constexpr Dir3 k_axis_y{0.0f, 1.0f, 0.0f};
inline constexpr Dir3 k_axis_z{0.0f, 0.0f, 1.0f};
}  // namespace ds_pba
