// pba/physics/collision.cpp
#include "pba/core/arena_allocator.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/physics/collision.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
//
#include <algorithm>
#include <array>
#include <cmath>
#include <gsl/assert>
#include <limits>
//
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>

namespace ds_pba
{

auto CollisionScratch::prepare(usize body_count) -> void
{
    aabb_min_x.resize(body_count);
    aabb_min_y.resize(body_count);
    aabb_min_z.resize(body_count);
    aabb_max_x.resize(body_count);
    aabb_max_y.resize(body_count);
    aabb_max_z.resize(body_count);

    is_static.resize(body_count);
    axes.resize(body_count);
    corners.resize(body_count);
    sort_order.resize(body_count);

    active.clear();
    candidate_pairs.clear();

    const auto wanted_candidates = body_count * 12zu;
    if (candidate_pairs.capacity() < wanted_candidates)
    {
        candidate_pairs.reserve(wanted_candidates);
    }
    if (active.capacity() < body_count)
    {
        active.reserve(body_count);
    }
}

namespace
{

static auto quantize_pos(f32 x, f32 cell) noexcept -> i32
{
    return static_cast<i32>(std::lround(static_cast<f64>(x / cell)));
}

static auto reduce_contact_points_to_4(
    std::array<Pos3, k_contact_points>& pts, usize& new_pt_count, const Dir3& n
) noexcept -> void
{
    if (new_pt_count <= 4)
    {
        return;
    }

    auto t1 = glm::cross(n, Dir3{1.0f, 0.0f, 0.0f});
    if (glm::dot(t1, t1) < 1e-8f)
    {
        t1 = glm::cross(n, Dir3{0.0f, 1.0f, 0.0f});
    }
    t1 = glm::normalize(t1);
    const auto t2 = glm::normalize(glm::cross(n, t1));

    auto pick_extremes = [&](const Dir3& axis) -> std::pair<usize, usize>
    {
        auto i_min = 0zu;
        auto i_max = 0zu;
        auto mn = glm::dot(pts[0], axis);
        auto mx = mn;

        for (auto i = 1zu; i < new_pt_count; ++i)
        {
            const auto d = glm::dot(pts[i], axis);
            if (d < mn)
            {
                mn = d;
                i_min = i;
            }
            if (d > mx)
            {
                mx = d;
                i_max = i;
            }
        }
        return {i_min, i_max};
    };

    const auto [a0, a1] = pick_extremes(t1);
    const auto [b0, b1] = pick_extremes(t2);

    auto idx = std::array<usize, 4>{a0, a1, b0, b1};

    auto reduced = std::array<Pos3, k_collision_reduced_num>{};
    auto reduced_count = 0zu;

    for (auto k = 0zu; k < idx.size(); ++k)
    {
        const auto p = pts[idx[k]];

        auto dup = false;
        for (auto r = 0zu; r < reduced_count; ++r)
        {
            const auto d = p - reduced[r];
            if (glm::dot(d, d) < 1e-8f)
            {
                dup = true;
                break;
            }
        }

        if (!dup)
        {
            reduced[reduced_count++] = p;
            if (reduced_count == 4)
            {
                break;
            }
        }
    }

    for (auto i = 0zu; i < reduced_count; ++i)
    {
        pts[i] = reduced[i];
    }
    new_pt_count = reduced_count;
}

auto obb_axes_world(const RigidBody& b) noexcept -> std::array<Dir3, 3>
{
    const auto R = glm::mat3_cast(b.orientation);
    return {
        Dir3{R[0]},
        Dir3{R[1]},
        Dir3{R[2]},
    };
}

auto box_world_corners(const RigidBody& b, const std::array<Dir3, 3>& axes) noexcept
    -> std::array<Pos3, 8>
{
    const auto ex = axes[0] * b.half_extents.x;
    const auto ey = axes[1] * b.half_extents.y;
    const auto ez = axes[2] * b.half_extents.z;
    return std::array<Pos3, 8>{
        b.position - ex - ey - ez,
        b.position - ex - ey + ez,
        b.position - ex + ey - ez,
        b.position - ex + ey + ez,
        b.position + ex - ey - ez,
        b.position + ex - ey + ez,
        b.position + ex + ey - ez,
        b.position + ex + ey + ez,
    };
}

auto world_aabb_half_extents(const RigidBody& b, const std::array<Dir3, 3>& axes) noexcept -> Dir3
{
    return Dir3{
        std::abs(axes[0].x) * b.half_extents.x + std::abs(axes[1].x) * b.half_extents.y
            + std::abs(axes[2].x) * b.half_extents.z,
        std::abs(axes[0].y) * b.half_extents.x + std::abs(axes[1].y) * b.half_extents.y
            + std::abs(axes[2].y) * b.half_extents.z,
        std::abs(axes[0].z) * b.half_extents.x + std::abs(axes[1].z) * b.half_extents.y
            + std::abs(axes[2].z) * b.half_extents.z,
    };
}

auto build_collision_soa(std::span<const RigidBody> bodies, CollisionScratch& scratch) -> void
{
    scratch.prepare(bodies.size());

    for (auto i = 0zu; i < bodies.size(); ++i)
    {
        const auto& b = bodies[i];
        const auto axes = obb_axes_world(b);
        const auto he = world_aabb_half_extents(b, axes);

        scratch.axes[i] = axes;
        scratch.corners[i] = box_world_corners(b, axes);

        scratch.aabb_min_x[i] = b.position.x - he.x;
        scratch.aabb_min_y[i] = b.position.y - he.y;
        scratch.aabb_min_z[i] = b.position.z - he.z;
        scratch.aabb_max_x[i] = b.position.x + he.x;
        scratch.aabb_max_y[i] = b.position.y + he.y;
        scratch.aabb_max_z[i] = b.position.z + he.z;

        scratch.is_static[i] = b.is_static() ? static_cast<u8>(1u) : static_cast<u8>(0u);
        scratch.sort_order[i] = i;
    }
}

auto generate_broadphase_candidates(CollisionScratch& scratch) -> void
{
    std::sort(
        scratch.sort_order.begin(),
        scratch.sort_order.end(),
        [&](usize lhs, usize rhs)
        {
            const auto a = scratch.aabb_min_x[lhs];
            const auto b = scratch.aabb_min_x[rhs];
            if (a < b)
            {
                return true;
            }
            if (a > b)
            {
                return false;
            }
            return lhs < rhs;
        }
    );

    scratch.active.clear();
    scratch.candidate_pairs.clear();

    for (const auto idx : scratch.sort_order)
    {
        const auto min_x = scratch.aabb_min_x[idx];

        auto write = 0zu;
        for (const auto active_idx : scratch.active)
        {
            if (scratch.aabb_max_x[active_idx] >= min_x)
            {
                scratch.active[write++] = active_idx;
            }
        }
        scratch.active.resize(write);

        for (const auto active_idx : scratch.active)
        {
            const auto is_static_static =
                (scratch.is_static[idx] != 0u) && (scratch.is_static[active_idx] != 0u);
            if (is_static_static)
            {
                continue;
            }

            const auto overlap_y = scratch.aabb_min_y[idx] <= scratch.aabb_max_y[active_idx]
                && scratch.aabb_max_y[idx] >= scratch.aabb_min_y[active_idx];
            if (!overlap_y)
            {
                continue;
            }

            const auto overlap_z = scratch.aabb_min_z[idx] <= scratch.aabb_max_z[active_idx]
                && scratch.aabb_max_z[idx] >= scratch.aabb_min_z[active_idx];
            if (!overlap_z)
            {
                continue;
            }

            const auto a = std::min(idx, active_idx);
            const auto b = std::max(idx, active_idx);
            scratch.candidate_pairs.push_back(CollisionPair{.a_idx = a, .b_idx = b});
        }

        scratch.active.push_back(idx);
    }
}

auto point_in_obb(const Pos3& p, const RigidBody& b, const std::array<Dir3, 3>& axes) noexcept
    -> bool
{
    const auto d = p - b.position;

    const auto lx = glm::dot(d, axes[0]);
    const auto ly = glm::dot(d, axes[1]);
    const auto lz = glm::dot(d, axes[2]);

    const auto he = b.half_extents;
    constexpr f32 eps{1e-6f};

    const auto inside_x = std::abs(lx) <= he.x + eps;
    const auto inside_y = std::abs(ly) <= he.y + eps;
    const auto inside_z = std::abs(lz) <= he.z + eps;
    return inside_x && inside_y && inside_z;
}

auto project_obb_on_axis(
    const RigidBody& b, const std::array<Dir3, 3>& axes, const Dir3& axis, f32& out_min, f32& out_max
) noexcept -> void
{
    const auto center_proj = glm::dot(b.position, axis);

    const auto radius_proj{
        std::abs(glm::dot(axes[0], axis)) * b.half_extents.x
        + std::abs(glm::dot(axes[1], axis)) * b.half_extents.y
        + std::abs(glm::dot(axes[2], axis)) * b.half_extents.z
    };

    out_min = center_proj - radius_proj;
    out_max = center_proj + radius_proj;
}

struct OverlapInfoObbObb
{
    Dir3 normal;
    f32 penetration;
    int axis_index;
};

auto obb_obb_overlap(
    const RigidBody& a,
    const std::array<Dir3, 3>& a_axes,
    const RigidBody& b,
    const std::array<Dir3, 3>& b_axes
) noexcept -> std::optional<OverlapInfoObbObb>
{
    auto axes = std::array<Dir3, 15>{};
    {
        axes[0] = a_axes[0];
        axes[1] = a_axes[1];
        axes[2] = a_axes[2];

        axes[3] = b_axes[0];
        axes[4] = b_axes[1];
        axes[5] = b_axes[2];

        auto k = 6zu;
        for (auto i = 0zu; i < 3zu; ++i)
        {
            for (auto j = 0zu; j < 3zu; ++j)
            {
                axes[k++] = glm::cross(a_axes[i], b_axes[j]);
            }
        }
    }

    const auto d = a.position - b.position;

    auto best_overlap_t = std::numeric_limits<f32>::infinity();
    auto best_axis = Dir3{0.0f, 0.0f, 1.0f};
    auto best_i = -1;

    for (auto i = 0zu; i < axes.size(); ++i)
    {
        const auto raw_axis = axes[i];
        const auto len2 = glm::dot(raw_axis, raw_axis);
        if (len2 <= 1e-10f)
        {
            continue;
        }

        const auto axis = raw_axis / std::sqrt(len2);

        auto a_min = 0.0f;
        auto a_max = 0.0f;
        project_obb_on_axis(a, a_axes, axis, a_min, a_max);
        auto b_min = 0.0f;
        auto b_max = 0.0f;
        project_obb_on_axis(b, b_axes, axis, b_min, b_max);

        const auto overlap = std::min(a_max, b_max) - std::max(a_min, b_min);
        if (overlap <= 0.0f)
        {
            return std::nullopt;
        }

        if (overlap < best_overlap_t)
        {
            best_overlap_t = overlap;
            const auto is_positive_axis_dir = glm::dot(d, axis) >= 0.0f;
            best_axis = (is_positive_axis_dir) ? axis : -axis;
            best_i = static_cast<int>(i);
        }
    }
    if (best_i < 0)
    {
        return std::nullopt;
    }
    return OverlapInfoObbObb{
        .normal = best_axis, .penetration = best_overlap_t, .axis_index = best_i
    };
}

auto emit_contacts_for_pair(
    std::span<const RigidBody> bodies,
    std::span<const std::array<Dir3, 3>> axes_cache,
    std::span<const std::array<Pos3, 8>> corners_cache,
    usize i,
    usize j,
    ArenaAllocator& out
) -> usize
{
    const auto& a = bodies[i];
    const auto& b = bodies[j];

    auto overlap_res = obb_obb_overlap(a, axes_cache[i], b, axes_cache[j]);
    if (!overlap_res)
    {
        return 0zu;
    }
    const auto& [normal, penetration, axis_index] = *overlap_res;
    const auto cross_axis = axis_index >= 6;

    auto pts = std::array<Pos3, k_contact_points>{};
    auto pt_count = 0zu;

    for (const auto& p : corners_cache[i])
    {
        if (pt_count >= pts.size())
        {
            break;
        }
        if (point_in_obb(p, b, axes_cache[j]))
        {
            pts[pt_count++] = p;
        }
    }

    for (const auto& p : corners_cache[j])
    {
        if (pt_count >= pts.size())
        {
            break;
        }
        if (point_in_obb(p, a, axes_cache[i]))
        {
            pts[pt_count++] = p;
        }
    }

    auto emitted = 0zu;
    if (pt_count == 0)
    {
        const auto mid = 0.5f * (a.position + b.position);
        const auto p = mid - 0.5f * penetration * normal;
        const auto* inserted = out.push_back(
            Contact{
                .a_idx = i,
                .b_idx = j,
                .p = p,
                .n = normal,
                .penetration = penetration,
                .allow_warm_start = false,
            }
        );
        if (inserted)
        {
            ++emitted;
        }
        return emitted;
    }

    reduce_contact_points_to_4(pts, pt_count, normal);

    for (auto k = 0zu; k < pt_count; ++k)
    {
        const auto* inserted = out.push_back(
            Contact{
                .a_idx = i,
                .b_idx = j,
                .p = pts[k],
                .n = normal,
                .penetration = penetration,
                .allow_warm_start = !cross_axis,
            }
        );
        if (inserted)
        {
            ++emitted;
        }
    }
    return emitted;
}

}  // namespace

auto make_contact_key(const RigidBody& a, const RigidBody& b, const Pos3& p) noexcept -> ContactKey
{
    constexpr auto k_cell = 0.02f;
    return ContactKey{
        .a_id = std::min(a.id, b.id),
        .b_id = std::max(a.id, b.id),
        .px = quantize_pos(p.x, k_cell),
        .py = quantize_pos(p.y, k_cell),
        .pz = quantize_pos(p.z, k_cell),
    };
}

auto generate_obb_contacts(
    std::span<const RigidBody> bodies, ArenaAllocator& out, CollisionScratch& scratch
) -> CollisionStats
{
    {
        Expects(out.used() == 0zu);
    }

    auto stats = CollisionStats{
        .body_count = bodies.size(),
    };
    if (bodies.size() < 2zu)
    {
        return stats;
    }

    build_collision_soa(bodies, scratch);
    generate_broadphase_candidates(scratch);
    stats.broadphase_candidates = scratch.candidate_pairs.size();

    for (const auto& pair : scratch.candidate_pairs)
    {
        ++stats.narrowphase_pairs;
        stats.contacts_generated += emit_contacts_for_pair(
            bodies, scratch.axes, scratch.corners, pair.a_idx, pair.b_idx, out
        );
    }

    return stats;
}

}  // namespace ds_pba
