// pba/core/geometry.cpp
#include "pba/core/geometry.hpp"
//
#include <cmath>
//
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace ds_pba
{

glm::vec3 safe_normalize(glm::vec3 v) noexcept
{
    const auto len = glm::length(v);
    if (len <= 1e-12f)
    {
        return glm::vec3{0.0f, 0.0f, 1.0f};
    }
    return v / len;
}

bool is_normalized(const Dir3& v, f32 eps) noexcept
{
    return std::abs(glm::dot(v, v) - 1.0f) <= eps;
}

Pos3 ModelMatrix::transform_position(const Pos3& p) const noexcept
{
    return Pos3{m * glm::vec4(p, 1.0f)};
}

Dir3 ModelMatrix::transform_direction(const Dir3& v) const noexcept
{
    return Dir3{m * glm::vec4(v, 0.0f)};
}

NormalMatrix ModelMatrix::normal_matrix() const noexcept
{
    return NormalMatrix{glm::inverseTranspose(glm::mat3(m))};
}

WorldToModelMatrix ModelMatrix::world_to_model() const noexcept
{
    return WorldToModelMatrix{glm::inverse(m)};
}

Pos3 WorldToModelMatrix::transform_position(const Pos3& p) const noexcept
{
    return Pos3{m * glm::vec4(p, 1.0f)};
}

Dir3 WorldToModelMatrix::transform_direction(const Dir3& v) const noexcept
{
    return Dir3{m * glm::vec4(v, 0.0f)};
}

Dir3 NormalMatrix::transform_normal(const Dir3& n) const noexcept
{
    return Dir3{m * n};
}

Dir3 NormalMatrix::transform_normal_unit(const Dir3& n) const noexcept
{
    return safe_normalize(transform_normal(n));
}

Pos3 ClipToWorldMatrix::unproject_ndc(f32 x_ndc, f32 y_ndc, f32 z_ndc) const noexcept
{
    glm::vec4 p = m * glm::vec4{x_ndc, y_ndc, z_ndc, 1.0f};
    p /= p.w;
    return Pos3{p};
}

ClipToWorldMatrix clip_to_world(const ProjMatrix& P, const ViewMatrix& V) noexcept
{
    return ClipToWorldMatrix{glm::inverse(P.m * V.m)};
}

}  // namespace ds_pba
