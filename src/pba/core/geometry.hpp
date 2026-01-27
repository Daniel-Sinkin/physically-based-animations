// pba/core/geometry.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
//
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/matrix_float4x4.hpp>

namespace ds_pba
{
template <class T>
    requires(!std::is_same_v<std::remove_cvref_t<T>, Color3>)
[[nodiscard]] constexpr auto lerp(const T& a, const T& b, f32 t) noexcept -> std::remove_cvref_t<T>
{
    using R = std::remove_cvref_t<T>;
    return static_cast<R>(a + (b - a) * t);
}

[[nodiscard]] constexpr Color3 mix(const Color3& a, const Color3& b, f32 t) noexcept
{
    return Color3{
        a.r() * (1.0f - t) + b.r() * t,
        a.g() * (1.0f - t) + b.g() * t,
        a.b() * (1.0f - t) + b.b() * t
    };
}

[[nodiscard]] glm::vec3 safe_normalize(glm::vec3 v) noexcept;
[[nodiscard]] bool is_normalized(const Dir3& v, f32 eps = 1e-4f) noexcept;

struct NormalMatrix;
struct WorldToModelMatrix;

struct ModelMatrix
{
    glm::mat4 m{glm::identity<glm::mat4>()};

    constexpr ModelMatrix() noexcept = default;
    explicit constexpr ModelMatrix(const glm::mat4& v) noexcept : m{v}
    {
    }

    [[nodiscard]] Pos3 transform_position(const Pos3& p) const noexcept;
    [[nodiscard]] Dir3 transform_direction(const Dir3& v) const noexcept;

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

    [[nodiscard]] Pos3 transform_position(const Pos3& p) const noexcept;
    [[nodiscard]] Dir3 transform_direction(const Dir3& v) const noexcept;
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

    [[nodiscard]] Pos3 unproject_ndc(f32 x_ndc, f32 y_ndc, f32 z_ndc) const noexcept;
};

[[nodiscard]] ClipToWorldMatrix clip_to_world(const ProjMatrix& P, const ViewMatrix& V) noexcept;

struct NormalMatrix
{
    glm::mat3 m{glm::identity<glm::mat3>()};

    constexpr NormalMatrix() noexcept = default;
    explicit constexpr NormalMatrix(const glm::mat3& v) noexcept : m{v}
    {
    }

    [[nodiscard]] Dir3 transform_normal(const Dir3& n) const noexcept;
    [[nodiscard]] Dir3 transform_normal_unit(const Dir3& n) const noexcept;
};
}  // namespace ds_pba
