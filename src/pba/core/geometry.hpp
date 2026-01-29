// pba/core/geometry.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
//
#include <algorithm>
#include <type_traits>
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

// Axis Aligned Bounding Box
struct AABB
{
    Pos3 min;
    Pos3 max;

    [[nodiscard]] static AABB empty() noexcept
    {
        return AABB{
            .min = Pos3{+k_inf, +k_inf, +k_inf},
            .max = Pos3{-k_inf, -k_inf, -k_inf},
        };
    }

    [[nodiscard]] static constexpr AABB unit() noexcept
    {
        auto h = 0.5f;
        return AABB{
            .min = Pos3{-h, -h, -h},
            .max = Pos3{h, h, h},
        };
    }

    [[nodiscard]] static constexpr AABB unit_positive() noexcept
    {
        return AABB{
            .min = Pos3{0.0f, 0.0f, 0.0f},
            .max = Pos3{1.0f, 1.0f, 1.0f},
        };
    }

    [[nodiscard]] static AABB
    from_center_half_extents(const Pos3& center, const Dir3& half_extents) noexcept
    {
        return AABB{
            .min = center - half_extents,
            .max = center + half_extents,
        };
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    [[nodiscard]] Pos3 center() const noexcept
    {
        return Pos3{0.5f * (min + max)};
    }

    [[nodiscard]] Dir3 extents() const noexcept
    {
        return Dir3{max - min};
    }

    [[nodiscard]] Dir3 half_extents() const noexcept
    {
        return Dir3{0.5f * (max - min)};
    }

    [[nodiscard]] f32 surface_area() const noexcept
    {
        const Dir3 s = extents();
        auto face_xy = s.x * s.y;
        auto face_xz = s.x * s.z;
        auto face_yz = s.y * s.z;
        return 2.0f * (face_xy + face_xz + face_yz);
    }

    [[nodiscard]] f32 volume() const noexcept
    {
        const Dir3 s = extents();
        return s.x * s.y * s.z;
    }
};

inline void expand_to_include(AABB& aabb, const Pos3& p) noexcept
{
    aabb.min.x = std::min(aabb.min.x, p.x);
    aabb.min.y = std::min(aabb.min.y, p.y);
    aabb.min.z = std::min(aabb.min.z, p.z);

    aabb.max.x = std::max(aabb.max.x, p.x);
    aabb.max.y = std::max(aabb.max.y, p.y);
    aabb.max.z = std::max(aabb.max.z, p.z);
}

inline void expand_to_include(AABB& aabb, const AABB& other) noexcept
{
    expand_to_include(aabb, other.min);
    expand_to_include(aabb, other.max);
}

[[nodiscard]] inline bool contains(const AABB& aabb, const Pos3& p) noexcept
{
    auto x_axis_overlap = p.x >= aabb.min.x && p.x <= aabb.max.x;
    auto y_axis_overlap = p.y >= aabb.min.y && p.y <= aabb.max.y;
    auto z_axis_overlap = p.z >= aabb.min.z && p.z <= aabb.max.z;
    return x_axis_overlap && y_axis_overlap && z_axis_overlap;
}

[[nodiscard]] inline bool contains(const AABB& outer, const AABB& inner) noexcept
{
    return contains(outer, inner.min) && contains(outer, inner.max);
}

[[nodiscard]] inline bool overlaps(const AABB& a, const AABB& b) noexcept
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
