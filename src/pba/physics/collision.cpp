// pba/physics/collision.cpp
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
#include <limits>
//
#include <glm/geometric.hpp>

namespace ds_pba
{
namespace
{

[[nodiscard]] static i32 quantize_pos(f32 x, f32 cell) noexcept
{
    return static_cast<i32>(std::lround(static_cast<f64>(x / cell)));
}

static void reduce_contact_points_4(
    std::array<Position3, k_contact_points>& pts, usize& pt_count, const Direction3& n
) noexcept
{
    if (pt_count <= 4)
    {
        return;
    }

    Direction3 t1{glm::cross(n, Direction3{1.0f, 0.0f, 0.0f})};
    if (glm::dot(t1, t1) < 1e-8f)
    {
        t1 = glm::cross(n, Direction3{0.0f, 1.0f, 0.0f});
    }
    t1 = glm::normalize(t1);
    const Direction3 t2{glm::normalize(glm::cross(n, t1))};

    auto pick_extremes = [&](const Direction3& axis) -> std::pair<usize, usize>
    {
        usize i_min{0zu};
        usize i_max{0zu};
        f32 mn = glm::dot(pts[0], axis);
        f32 mx = mn;

        for (usize i{1zu}; i < pt_count; ++i)
        {
            const f32 d{glm::dot(pts[i], axis)};
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

    std::array<usize, 4> idx{a0, a1, b0, b1};

    std::array<Position3, k_collision_reduced_num> reduced{};
    usize reduced_count{0zu};

    for (usize k{0zu}; k < idx.size(); ++k)
    {
        const Position3 p{pts[idx[k]]};

        bool dup{false};
        for (usize r{0zu}; r < reduced_count; ++r)
        {
            const Direction3 d{p - reduced[r]};
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

    for (usize i{0zu}; i < reduced_count; ++i)
    {
        pts[i] = reduced[i];
    }
    pt_count = reduced_count;
}

[[nodiscard]] glm::mat3 R_from_q(const Quaternion& q) noexcept
{
    return glm::mat3_cast(q);
}

[[nodiscard]] std::array<Direction3, 3> obb_axes_world(const RigidBody& b) noexcept
{
    const glm::mat3 R{R_from_q(b.orientation)};

    return {
        glm::normalize(R * k_axis_x),
        glm::normalize(R * k_axis_y),
        glm::normalize(R * k_axis_z),
    };
}

[[nodiscard]] std::array<Position3, 8> box_world_corners(const RigidBody& b) noexcept
{
    const auto [ax, ay, az] = obb_axes_world(b);
    const Direction3 ex{ax * b.half_extents.x};
    const Direction3 ey{ay * b.half_extents.y};
    const Direction3 ez{az * b.half_extents.z};
    return std::array<Position3, 8>{
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

[[nodiscard]] bool point_in_obb(const Position3& p, const RigidBody& b) noexcept
{
    const std::array<Direction3, 3> axes = obb_axes_world(b);
    const Direction3 d{p - b.position};

    const f32 lx{glm::dot(d, axes[0])};
    const f32 ly{glm::dot(d, axes[1])};
    const f32 lz{glm::dot(d, axes[2])};

    const Direction3 he{b.half_extents};
    constexpr f32 eps{1e-6f};

    const bool inside_x{std::abs(lx) <= he.x + eps};
    const bool inside_y{std::abs(ly) <= he.y + eps};
    const bool inside_z{std::abs(lz) <= he.z + eps};

    return inside_x && inside_y && inside_z;
}

void project_obb_on_axis(
    const RigidBody& b, const Direction3& axis, f32& out_min, f32& out_max
) noexcept
{
    const auto axes = obb_axes_world(b);

    const f32 center_proj{glm::dot(b.position, axis)};

    const f32 radius_proj{
        std::abs(glm::dot(axes[0], axis)) * b.half_extents.x
        + std::abs(glm::dot(axes[1], axis)) * b.half_extents.y
        + std::abs(glm::dot(axes[2], axis)) * b.half_extents.z
    };

    out_min = center_proj - radius_proj;
    out_max = center_proj + radius_proj;
}

[[nodiscard]] bool sat_obb_obb(
    const RigidBody& a,
    const RigidBody& b,
    Direction3& out_n,
    f32& out_penetration,
    int& out_axis_index
) noexcept
{
    const auto ax = obb_axes_world(a);
    const auto bx = obb_axes_world(b);

    std::array<Direction3, 15> axes{};
    {
        axes[0] = ax[0];
        axes[1] = ax[1];
        axes[2] = ax[2];

        axes[3] = bx[0];
        axes[4] = bx[1];
        axes[5] = bx[2];

        usize k{6zu};
        for (usize i{0zu}; i < 3zu; ++i)
        {
            for (usize j{0zu}; j < 3zu; ++j)
            {
                axes[k++] = glm::cross(ax[i], bx[j]);
            }
        }
    }

    const Direction3 d{a.position - b.position};

    f32 best_overlap{std::numeric_limits<f32>::infinity()};
    Direction3 best_axis{0.0f, 0.0f, 1.0f};
    int best_i{-1};

    for (int i{0}; i < static_cast<int>(axes.size()); ++i)
    {
        const Direction3 raw_axis{axes[static_cast<usize>(i)]};
        const f32 len2 = glm::dot(raw_axis, raw_axis);
        if (len2 <= 1e-10f)
        {
            continue;
        }

        const Direction3 axis = raw_axis / std::sqrt(len2);

        f32 a_min{}, a_max{};
        f32 b_min{}, b_max{};
        project_obb_on_axis(a, axis, a_min, a_max);
        project_obb_on_axis(b, axis, b_min, b_max);

        const f32 overlap{std::min(a_max, b_max) - std::max(a_min, b_min)};
        if (overlap <= 0.0f)
        {
            return false;
        }

        if (overlap < best_overlap)
        {
            best_overlap = overlap;

            // Ensure normal points from b -> a
            const f32 s{glm::dot(d, axis)};
            best_axis = (s >= 0.0f) ? axis : -axis;

            best_i = i;
        }
    }
    if (best_i < 0)
    {
        return false;
    }

    out_n = best_axis;
    out_penetration = best_overlap;
    out_axis_index = best_i;
    return true;
}

}  // namespace

ContactKey make_contact_key(const RigidBody& a, const RigidBody& b, const Position3& p) noexcept
{
    const ObjectId id0 = std::min(a.id, b.id);
    const ObjectId id1 = std::max(a.id, b.id);

    constexpr f32 k_cell = 0.02f;

    return ContactKey{
        .a_id = id0,
        .b_id = id1,
        .px = quantize_pos(p.x, k_cell),
        .py = quantize_pos(p.y, k_cell),
        .pz = quantize_pos(p.z, k_cell),
    };
}

void generate_obb_contacts(std::span<const RigidBody> bodies, std::pmr::vector<Contact>& out)
{
    out.clear();
    out.reserve(bodies.size() * 8zu);  // TODO: Profile what a good default would be

    for (usize i{0zu}; i < bodies.size(); ++i)
    {
        for (usize j{i + 1zu}; j < bodies.size(); ++j)
        {
            const RigidBody& a{bodies[i]};
            const RigidBody& b{bodies[j]};

            if (a.is_static() && b.is_static())
            {
                continue;
            }

            Direction3 n{0.0f, 0.0f, 1.0f};
            f32 penetration{0.0f};
            int axis_index{-1};
            if (!sat_obb_obb(a, b, n, penetration, axis_index))
            {
                continue;
            }
            const bool cross_axis{axis_index >= 6};

            std::array<Position3, k_contact_points> pts{};
            usize pt_count{0};

            const std::array<Position3, 8> a_corners{box_world_corners(a)};
            for (const Position3& p : a_corners)
            {
                if (point_in_obb(p, b))
                {
                    if (pt_count < pts.size())
                    {
                        pts[pt_count++] = p;
                    }
                }
            }

            const std::array<Position3, 8> b_corners{box_world_corners(b)};
            for (const Position3& p : b_corners)
            {
                if (point_in_obb(p, a))
                {
                    if (pt_count < pts.size())
                    {
                        pts[pt_count++] = p;
                    }
                }
            }

            if (pt_count == 0)
            {
                const Position3 mid{0.5f * (a.position + b.position)};
                const Position3 p{mid - 0.5f * penetration * n};
                Contact c{
                    .a_idx = i,
                    .b_idx = j,
                    .p = p,
                    .n = n,
                    .penetration = penetration,
                    .allow_warm_start = false,
                };

                out.push_back(c);
                continue;
            }

            reduce_contact_points_4(pts, pt_count, n);

            for (usize k{0zu}; k < pt_count; ++k)
            {
                Contact c{
                    .a_idx = i,
                    .b_idx = j,
                    .p = pts[k],  // TODO: This is not entirey stable after reducing, replace
                                  // once stable manifold ids are implemented
                    .n = n,
                    .penetration = penetration,
                    .allow_warm_start = !cross_axis,
                };
                out.push_back(c);
            }
        }
    }
}

}  // namespace ds_pba
