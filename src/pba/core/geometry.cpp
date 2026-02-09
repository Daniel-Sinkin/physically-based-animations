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
#include <optional>

namespace ds_pba
{

auto safe_normalize(glm::vec3 v) noexcept -> glm::vec3
{
    const auto len = glm::length(v);
    if (len <= 1e-12f)
    {
        return glm::vec3{0.0f, 0.0f, 1.0f};
    }
    return v / len;
}

auto normalize(glm::vec3 v) -> std::optional<glm::vec3>
{
    const auto len = glm::length(v);
    if (len <= 1e-12f)
    {
        return std::nullopt;
    }
    return v / len;
}

auto is_normalized(const Dir3& v, f32 eps) noexcept -> bool
{
    return std::abs(glm::dot(v, v) - 1.0f) <= eps;
}

auto ModelMatrix::transform_position(const Pos3& p) const noexcept -> Pos3
{
    return Pos3{m * glm::vec4(p, 1.0f)};
}

auto ModelMatrix::transform_direction(const Dir3& v) const noexcept -> Dir3
{
    return Dir3{m * glm::vec4(v, 0.0f)};
}

auto ModelMatrix::normal_matrix() const noexcept -> NormalMatrix
{
    return NormalMatrix{glm::inverseTranspose(glm::mat3(m))};
}

auto ModelMatrix::world_to_model() const noexcept -> WorldToModelMatrix
{
    return WorldToModelMatrix{glm::inverse(m)};
}

auto WorldToModelMatrix::transform_position(const Pos3& p) const noexcept -> Pos3
{
    return Pos3{m * glm::vec4(p, 1.0f)};
}

auto WorldToModelMatrix::transform_direction(const Dir3& v) const noexcept -> Dir3
{
    return Dir3{m * glm::vec4(v, 0.0f)};
}

auto NormalMatrix::transform_normal(const Dir3& n) const noexcept -> Dir3
{
    return Dir3{m * n};
}

auto NormalMatrix::transform_normal_unit(const Dir3& n) const noexcept -> Dir3
{
    return safe_normalize(transform_normal(n));
}

auto ClipToWorldMatrix::unproject_ndc(f32 x_ndc, f32 y_ndc, f32 z_ndc) const noexcept -> Pos3
{
    auto p = m * glm::vec4{x_ndc, y_ndc, z_ndc, 1.0f};
    p /= p.w;
    return Pos3{p};
}

auto clip_to_world(const ProjMatrix& P, const ViewMatrix& V) noexcept -> ClipToWorldMatrix
{
    return ClipToWorldMatrix{glm::inverse(P.m * V.m)};
}

}  // namespace ds_pba
