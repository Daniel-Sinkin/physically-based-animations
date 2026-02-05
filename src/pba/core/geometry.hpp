// pba/core/geometry.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
//
#include <algorithm>
//
#include <glm/ext/matrix_float3x3.hpp>
#include <glm/ext/matrix_float4x4.hpp>

namespace ds_pba
{
[[nodiscard]] constexpr auto mix(const Color3& a, const Color3& b, f32 t) noexcept -> Color3
{
    return Color3{
        a.r() * (1.0f - t) + b.r() * t,
        a.g() * (1.0f - t) + b.g() * t,
        a.b() * (1.0f - t) + b.b() * t
    };
}

[[nodiscard]] auto safe_normalize(glm::vec3 v) noexcept -> glm ::vec3;
[[nodiscard]] auto is_normalized(const Dir3& v, f32 eps = 1e-4f) noexcept -> bool;

// Axis Aligned Bounding Box
struct AABB
{
    Pos3 min;
    Pos3 max;

    [[nodiscard]] static auto get_empty() noexcept -> AABB
    {
        return AABB{
            .min = Pos3{+k_inf, +k_inf, +k_inf},
            .max = Pos3{-k_inf, -k_inf, -k_inf},
        };
    }

    [[nodiscard]] static constexpr auto unit() noexcept -> AABB
    {
        auto h = 0.5f;
        return AABB{
            .min = Pos3{-h, -h, -h},
            .max = Pos3{h, h, h},
        };
    }

    [[nodiscard]] static constexpr auto unit_positive() noexcept -> AABB
    {
        return AABB{
            .min = Pos3{0.0f, 0.0f, 0.0f},
            .max = Pos3{1.0f, 1.0f, 1.0f},
        };
    }

    [[nodiscard]] static auto
    from_center_half_extents(const Pos3& center, const Dir3& half_extents) noexcept -> AABB
    {
        return AABB{
            .min = center - half_extents,
            .max = center + half_extents,
        };
    }

    [[nodiscard]] auto valid() const noexcept -> bool
    {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    [[nodiscard]] auto center() const noexcept -> Pos3
    {
        return Pos3{0.5f * (min + max)};
    }

    [[nodiscard]] auto extents() const noexcept -> Dir3
    {
        return Dir3{max - min};
    }

    [[nodiscard]] auto half_extents() const noexcept -> Dir3
    {
        return Dir3{0.5f * (max - min)};
    }

    [[nodiscard]] auto surface_area() const noexcept -> f32
    {
        const Dir3 s = extents();
        auto face_xy = s.x * s.y;
        auto face_xz = s.x * s.z;
        auto face_yz = s.y * s.z;
        return 2.0f * (face_xy + face_xz + face_yz);
    }

    [[nodiscard]] auto volume() const noexcept -> f32
    {
        const Dir3 s = extents();
        return s.x * s.y * s.z;
    }
};

inline auto expand_to_include(AABB& aabb, const Pos3& p) noexcept -> void
{
    aabb.min.x = std::min(aabb.min.x, p.x);
    aabb.min.y = std::min(aabb.min.y, p.y);
    aabb.min.z = std::min(aabb.min.z, p.z);

    aabb.max.x = std::max(aabb.max.x, p.x);
    aabb.max.y = std::max(aabb.max.y, p.y);
    aabb.max.z = std::max(aabb.max.z, p.z);
}

inline auto expand_to_include(AABB& aabb, const AABB& other) noexcept -> void
{
    expand_to_include(aabb, other.min);
    expand_to_include(aabb, other.max);
}

[[nodiscard]] inline auto contains(const AABB& aabb, const Pos3& p) noexcept -> bool
{
    auto x_axis_overlap = p.x >= aabb.min.x && p.x <= aabb.max.x;
    auto y_axis_overlap = p.y >= aabb.min.y && p.y <= aabb.max.y;
    auto z_axis_overlap = p.z >= aabb.min.z && p.z <= aabb.max.z;
    return x_axis_overlap && y_axis_overlap && z_axis_overlap;
}

[[nodiscard]] inline auto contains(const AABB& outer, const AABB& inner) noexcept -> bool
{
    return contains(outer, inner.min) && contains(outer, inner.max);
}

[[nodiscard]] inline auto overlaps(const AABB& a, const AABB& b) noexcept -> bool
{
    auto x_axis_overlap = a.min.x <= b.max.x && a.max.x >= b.min.x;
    auto y_axis_overlap = a.min.y <= b.max.y && a.max.y >= b.min.y;
    auto z_axis_overlap = a.min.z <= b.max.z && a.max.z >= b.min.z;
    return x_axis_overlap && y_axis_overlap && z_axis_overlap;
}

struct NormalMatrix;
struct WorldToModelMatrix;

struct ModelMatrix
{
    glm::mat4 m{glm::identity<glm::mat4>()};

    constexpr ModelMatrix() noexcept = default;
    explicit constexpr ModelMatrix(const glm::mat4& v) noexcept : m{v}
    {
    }

    [[nodiscard]] auto transform_position(const Pos3& p) const noexcept -> Pos3;
    [[nodiscard]] auto transform_direction(const Dir3& v) const noexcept -> Dir3;
    [[nodiscard]] auto normal_matrix() const noexcept -> NormalMatrix;
    [[nodiscard]] auto world_to_model() const noexcept -> WorldToModelMatrix;
};

struct WorldToModelMatrix
{
    glm::mat4 m{glm::identity<glm::mat4>()};

    constexpr WorldToModelMatrix() noexcept = default;
    explicit constexpr WorldToModelMatrix(const glm::mat4& v) noexcept : m{v}
    {
    }

    [[nodiscard]] auto transform_position(const Pos3& p) const noexcept -> Pos3;
    [[nodiscard]] auto transform_direction(const Dir3& v) const noexcept -> Dir3;
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

    [[nodiscard]] auto unproject_ndc(f32 x_ndc, f32 y_ndc, f32 z_ndc) const noexcept -> Pos3;
};

[[nodiscard]] auto clip_to_world(const ProjMatrix& P, const ViewMatrix& V) noexcept
    -> ClipToWorldMatrix;

struct NormalMatrix
{
    glm::mat3 m{glm::identity<glm::mat3>()};

    constexpr NormalMatrix() noexcept = default;
    explicit constexpr NormalMatrix(const glm::mat3& v) noexcept : m{v}
    {
    }

    [[nodiscard]] auto transform_normal(const Dir3& n) const noexcept -> Dir3;
    [[nodiscard]] auto transform_normal_unit(const Dir3& n) const noexcept -> Dir3;
};
}  // namespace ds_pba
