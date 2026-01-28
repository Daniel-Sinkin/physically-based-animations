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
#include <glm/gtc/quaternion.hpp>

namespace ds_pba
{
namespace
{

static i32 quantize_pos(f32 x, f32 cell) noexcept
{
    return static_cast<i32>(std::lround(static_cast<f64>(x / cell)));
}

static void reduce_contact_points_to_4(
    std::array<Pos3, k_contact_points>& pts, usize& new_pt_count, const Dir3& n
) noexcept
{
    if (new_pt_count <= 4)
    {
        return;
    }

    Dir3 t1{glm::cross(n, Dir3{1.0f, 0.0f, 0.0f})};
    if (glm::dot(t1, t1) < 1e-8f)
    {
        t1 = glm::cross(n, Dir3{0.0f, 1.0f, 0.0f});
    }
    t1 = glm::normalize(t1);
    const Dir3 t2{glm::normalize(glm::cross(n, t1))};

    auto pick_extremes = [&](const Dir3& axis) -> std::pair<usize, usize>
    {
        usize i_min{0zu};
        usize i_max{0zu};
        f32 mn = glm::dot(pts[0], axis);
        f32 mx = mn;

        for (usize i{1zu}; i < new_pt_count; ++i)
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

    std::array<Pos3, k_collision_reduced_num> reduced{};
    usize reduced_count{0zu};

    for (usize k{0zu}; k < idx.size(); ++k)
    {
        const Pos3 p{pts[idx[k]]};

        bool dup{false};
        for (usize r{0zu}; r < reduced_count; ++r)
        {
            const Dir3 d{p - reduced[r]};
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
    new_pt_count = reduced_count;
}

std::array<Dir3, 3> obb_axes_world(const RigidBody& b) noexcept
{
    // In an AABB the faces are already aligned with the standard basis so rotating
    // an AABB and rotating the local axis is the same thing.
    const auto R = glm::mat3_cast(b.orientation);
    // GLM (and OpenGL) store array in column major
    return {
        Dir3{R[0]},
        Dir3{R[1]},
        Dir3{R[2]},
    };
}

std::array<Pos3, 8> box_world_corners(const RigidBody& b) noexcept
{
    const auto [ax, ay, az] = obb_axes_world(b);
    // From center of mass move towards one of the faces == move along rotated axis
    // so we just need to do one step of length of the half extent.
    const Dir3 ex{ax * b.half_extents.x};
    const Dir3 ey{ay * b.half_extents.y};
    const Dir3 ez{az * b.half_extents.z};
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

bool point_in_obb(const Pos3& p, const RigidBody& b) noexcept
{
    const std::array<Dir3, 3> axes = obb_axes_world(b);
    const Dir3 d{p - b.position};

    const f32 lx{glm::dot(d, axes[0])};
    const f32 ly{glm::dot(d, axes[1])};
    const f32 lz{glm::dot(d, axes[2])};

    const Dir3 he{b.half_extents};
    constexpr f32 eps{1e-6f};

    const bool inside_x{std::abs(lx) <= he.x + eps};
    const bool inside_y{std::abs(ly) <= he.y + eps};
    const bool inside_z{std::abs(lz) <= he.z + eps};
    return inside_x && inside_y && inside_z;
}

void project_obb_on_axis(const RigidBody& b, const Dir3& axis, f32& out_min, f32& out_max) noexcept
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

bool obb_obb_overlap(
    const RigidBody& a, const RigidBody& b, Dir3& out_n, f32& out_penetration, int& out_axis_index
) noexcept
{  // Uses Seperating Axis Theorem (SAT)
    const auto ax = obb_axes_world(a);
    const auto bx = obb_axes_world(b);

    std::array<Dir3, 15> axes{};
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

    const Dir3 d{a.position - b.position};

    f32 best_overlap{std::numeric_limits<f32>::infinity()};
    Dir3 best_axis{0.0f, 0.0f, 1.0f};
    int best_i{-1};

    for (usize i{0}; i < axes.size(); ++i)
    {
        const Dir3 raw_axis{axes[i]};
        const f32 len2 = glm::dot(raw_axis, raw_axis);
        if (len2 <= 1e-10f)
        {
            continue;
        }

        const Dir3 axis = raw_axis / std::sqrt(len2);

        f32 a_min{}, a_max{};
        project_obb_on_axis(a, axis, a_min, a_max);
        f32 b_min{}, b_max{};
        project_obb_on_axis(b, axis, b_min, b_max);

        const f32 overlap{std::min(a_max, b_max) - std::max(a_min, b_min)};
        if (overlap <= 0.0f)
        {
            return false;
        }

        if (overlap < best_overlap)
        {
            best_overlap = overlap;
            const f32 s{glm::dot(d, axis)};
            best_axis = (s >= 0.0f) ? axis : -axis;
            best_i = static_cast<int>(i);
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

ContactKey make_contact_key(const RigidBody& a, const RigidBody& b, const Pos3& p) noexcept
{
    const ObjectId id0 = std::min(a.id, b.id);
    const ObjectId id1 = std::max(a.id, b.id);

    constexpr auto k_cell = 0.02f;

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
    Expects(out.empty() && "arena allocator should be wiped at start of iteration");
    out.reserve(bodies.size() * 8zu);  // TODO: Profile what a good default would be

    for (usize i{0zu}; i < bodies.size(); ++i)
    {
        for (usize j{i + 1zu}; j < bodies.size(); ++j)
        {
            const RigidBody& a{bodies[i]};
            const RigidBody& b{bodies[j]};
            // Static & Static might intersect but there is no force being generated
            if (a.is_static() && b.is_static())
            {
                continue;
            }

            Dir3 n{k_axis_z};
            f32 penetration{0.0f};
            int axis_index{-1};
            if (!obb_obb_overlap(a, b, n, penetration, axis_index))
            {
                continue;
            }
            const bool cross_axis{axis_index >= 6};

            // More contact points are more expensive
            std::array<Pos3, k_contact_points> pts{};
            usize pt_count{0};

            // Get the obb corner positions
            const std::array<Pos3, 8> a_corners{box_world_corners(a)};
            for (const auto& p : a_corners)
            {
                if (pt_count >= pts.size())
                {
                    break;
                }
                if (point_in_obb(p, b))
                {
                    pts[pt_count++] = p;
                }
            }

            const std::array<Pos3, 8> b_corners{box_world_corners(b)};
            for (const auto& p : b_corners)
            {
                if (pt_count >= pts.size())
                {
                    break;
                }
                if (point_in_obb(p, a))
                {
                    pts[pt_count++] = p;
                }
            }

            if (pt_count == 0)
            {
                // TODO: Replace this by a more sophisticated heuristic

                // If we don't find any contact points we create a
                // virtual contact point in the middle of the two
                // center of masses. This is a pretty bad heuristic
                // in particular if one of (or both) the objects
                // is scaled very large as the mid point can be quite
                // far away.
                const Pos3 mid{0.5f * (a.position + b.position)};
                const Pos3 p{mid - 0.5f * penetration * n};
                out.push_back(
                    Contact{
                        .a_idx = i,
                        .b_idx = j,
                        .p = p,
                        .n = n,
                        .penetration = penetration,
                        .allow_warm_start = false,
                    }
                );
                continue;
            }
            reduce_contact_points_to_4(pts, pt_count, n);

            for (usize k{0zu}; k < pt_count; ++k)
            {
                // TODO: This is not entirey stable after reducing, replace
                // once stable manifold ids are implemented
                out.push_back(
                    Contact{
                        .a_idx = i,
                        .b_idx = j,
                        .p = pts[k],
                        .n = n,
                        .penetration = penetration,
                        .allow_warm_start = !cross_axis,
                    }
                );
            }
        }
    }
}

}  // namespace ds_pba
