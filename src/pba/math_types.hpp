// pba/math_types.hpp
#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"

namespace ds_pba
{
using ModelMatrix = glm::mat4;
using ViewMatrix = glm::mat4;
using ProjMatrix = glm::mat4;

using Position3 = glm::vec3;
using Direction3 = glm::vec3;
using EulerDeg3 = glm::vec3;

using ColorRGBf = glm::vec3;
}  // namespace ds_pba
