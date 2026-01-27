// pba/core/geometry.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"

#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/matrix_float4x4.hpp>

namespace ds_pba
{
[[nodiscard]] glm::vec3 safe_normalize(glm::vec3 v) noexcept;
[[nodiscard]] bool is_normalized(const Direction3& v, f32 eps = 1e-4f) noexcept;

struct NormalMatrix;
struct WorldToModelMatrix;

struct ModelMatrix
{
    glm::mat4 m{glm::identity<glm::mat4>()};

    constexpr ModelMatrix() noexcept = default;
    explicit constexpr ModelMatrix(const glm::mat4& v) noexcept : m{v}
    {
    }

    [[nodiscard]] Position3 transform_position(const Position3& p) const noexcept;
    [[nodiscard]] Direction3 transform_direction(const Direction3& v) const noexcept;

    [[nodiscard]] NormalMatrix normal_matrix() const noexcept;
    [[nodiscard]] WorldToModelMatrix world_to_model() const noexcept;
};

struct WorldToModelMatrix
{
    glm::mat4 m{glm::identity<glm::mat4>()};

    constexpr WorldToModelMatrix() noexcept = default;
    explicit constexpr WorldToModelMatrix(const glm::mat4& v) noexcept : m{v}
    {
    }

    [[nodiscard]] Position3 transform_position(const Position3& p) const noexcept;
    [[nodiscard]] Direction3 transform_direction(const Direction3& v) const noexcept;
};

struct ViewMatrix
{
    glm::mat4 m{glm::identity<glm::mat4>()};

    constexpr ViewMatrix() noexcept = default;
    explicit constexpr ViewMatrix(const glm::mat4& v) noexcept : m{v}
    {
    }
};

struct ProjMatrix
{
    glm::mat4 m{glm::identity<glm::mat4>()};

    constexpr ProjMatrix() noexcept = default;
    explicit constexpr ProjMatrix(const glm::mat4& v) noexcept : m{v}
    {
    }
};

struct ClipToWorldMatrix
{
    glm::mat4 m{glm::identity<glm::mat4>()};

    constexpr ClipToWorldMatrix() noexcept = default;
    explicit constexpr ClipToWorldMatrix(const glm::mat4& v) noexcept : m{v}
    {
    }

    [[nodiscard]] Position3 unproject_ndc(f32 x_ndc, f32 y_ndc, f32 z_ndc) const noexcept;
};

[[nodiscard]] ClipToWorldMatrix clip_to_world(const ProjMatrix& P, const ViewMatrix& V) noexcept;

struct NormalMatrix
{
    glm::mat3 m{glm::identity<glm::mat3>()};

    constexpr NormalMatrix() noexcept = default;
    explicit constexpr NormalMatrix(const glm::mat3& v) noexcept : m{v}
    {
    }

    [[nodiscard]] Direction3 transform_normal(const Direction3& n) const noexcept;
    [[nodiscard]] Direction3 transform_normal_unit(const Direction3& n) const noexcept;
};
}  // namespace ds_pba
