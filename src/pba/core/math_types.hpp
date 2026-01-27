// pba/core/math_types.hpp
#pragma once

#include "pba/core/core_types.hpp"

#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/vector_float3.hpp>
#include <numbers>

namespace ds_pba
{
using Position3 = glm::vec3;
using Direction3 = glm::vec3;
using EulerDeg3 = glm::vec3;
using Quaternion = glm::quat;

using ColorRGBf = glm::vec3;

// Common math constants
inline constexpr Direction3 k_zero_dir{0.0f, 0.0f, 0.0f};
inline constexpr Quaternion k_quaternion_identity{1.0f, 0.0f, 0.0f, 0.0f};

inline constexpr f32 k_pi{std::numbers::pi_v<f32>};
inline constexpr f32 k_two_pi{2.0f * std::numbers::pi_v<f32>};

inline constexpr Direction3 k_axis_x{1.0f, 0.0f, 0.0f};
inline constexpr Direction3 k_axis_y{0.0f, 1.0f, 0.0f};
inline constexpr Direction3 k_axis_z{0.0f, 0.0f, 1.0f};
}  // namespace ds_pba
