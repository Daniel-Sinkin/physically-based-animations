// pba/assets/mesh.cpp
#include "pba/core/core_types.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/assets/mesh.hpp"
//
#include "pba/core/math_types.hpp"

#include <gsl/assert>

namespace ds_pba
{
namespace
{

template <usize N>
auto mesh_pn_from_array(const std::array<MeshV_PN, N>& a) -> MeshDataPN
{
    MeshDataPN out{};
    out.vertices.reserve(N);
    out.vertices.insert(out.vertices.end(), a.begin(), a.end());
    return out;
}

}  // namespace

auto create_cube_mesh() -> MeshDataPN
{
    constexpr auto h = 0.5f;
    // clang-format off
    static constexpr std::array<MeshV_PN, 36> verts = {
        // +Z
        MeshV_PN{-h, -h,  h,  0,  0,  1},
        MeshV_PN{ h, -h,  h,  0,  0,  1},
        MeshV_PN{ h,  h,  h,  0,  0,  1},
        MeshV_PN{-h, -h,  h,  0,  0,  1},
        MeshV_PN{ h,  h,  h,  0,  0,  1},
        MeshV_PN{-h,  h,  h,  0,  0,  1},
        // -Z
        MeshV_PN{-h, -h, -h,  0,  0, -1},
        MeshV_PN{ h,  h, -h,  0,  0, -1},
        MeshV_PN{ h, -h, -h,  0,  0, -1},
        MeshV_PN{-h, -h, -h,  0,  0, -1},
        MeshV_PN{-h,  h, -h,  0,  0, -1},
        MeshV_PN{ h,  h, -h,  0,  0, -1},
        // +X
        MeshV_PN{ h, -h, -h,  1,  0,  0},
        MeshV_PN{ h,  h, -h,  1,  0,  0},
        MeshV_PN{ h,  h,  h,  1,  0,  0},
        MeshV_PN{ h, -h, -h,  1,  0,  0},
        MeshV_PN{ h,  h,  h,  1,  0,  0},
        MeshV_PN{ h, -h,  h,  1,  0,  0},
        // -X
        MeshV_PN{-h, -h, -h, -1,  0,  0},
        MeshV_PN{-h,  h,  h, -1,  0,  0},
        MeshV_PN{-h,  h, -h, -1,  0,  0},
        MeshV_PN{-h, -h, -h, -1,  0,  0},
        MeshV_PN{-h, -h,  h, -1,  0,  0},
        MeshV_PN{-h,  h,  h, -1,  0,  0},
        // +Y
        MeshV_PN{-h,  h, -h,  0,  1,  0},
        MeshV_PN{ h,  h,  h,  0,  1,  0},
        MeshV_PN{ h,  h, -h,  0,  1,  0},
        MeshV_PN{-h,  h, -h,  0,  1,  0},
        MeshV_PN{-h,  h,  h,  0,  1,  0},
        MeshV_PN{ h,  h,  h,  0,  1,  0},
        // -Y
        MeshV_PN{-h, -h, -h,  0, -1,  0},
        MeshV_PN{ h, -h, -h,  0, -1,  0},
        MeshV_PN{ h, -h,  h,  0, -1,  0},
        MeshV_PN{-h, -h, -h,  0, -1,  0},
        MeshV_PN{ h, -h,  h,  0, -1,  0},
        MeshV_PN{-h, -h,  h,  0, -1,  0},
    };
    // clang-format on
    return mesh_pn_from_array(verts);
}

auto create_quad_mesh() -> MeshDataPN
{
    constexpr auto h = 0.5f;
    // clang-format off
    static constexpr std::array<MeshV_PN, 6> verts = {
        MeshV_PN{-h, -h, 0.0f,  0,  0,  1},
        MeshV_PN{ h, -h, 0.0f,  0,  0,  1},
        MeshV_PN{ h,  h, 0.0f,  0,  0,  1},
        MeshV_PN{-h, -h, 0.0f,  0,  0,  1},
        MeshV_PN{ h,  h, 0.0f,  0,  0,  1},
        MeshV_PN{-h,  h, 0.0f,  0,  0,  1},
    };
    // clang-format on

    return mesh_pn_from_array(verts);
}

auto create_pyramid_mesh() -> MeshDataPN
{
    static constexpr auto n_xy{0.8944271909999159f};  // 2 / sqrt(5)
    static constexpr auto n_z{0.4472135954999579f};   // 1 / sqrt(5)

    // clang-format off
    static constexpr std::array<MeshV_PN, 18> verts = {
        // Side faces
        // -Y face
        MeshV_PN{-0.5f, -0.5f, -0.5f,   0.0f, -n_xy,  n_z},
        MeshV_PN{ 0.5f, -0.5f, -0.5f,   0.0f, -n_xy,  n_z},
        MeshV_PN{ 0.0f,  0.0f,  0.5f,   0.0f, -n_xy,  n_z},

        // +X face
        MeshV_PN{ 0.5f, -0.5f, -0.5f,   n_xy,  0.0f,  n_z},
        MeshV_PN{ 0.5f,  0.5f, -0.5f,   n_xy,  0.0f,  n_z},
        MeshV_PN{ 0.0f,  0.0f,  0.5f,   n_xy,  0.0f,  n_z},

        // +Y face
        MeshV_PN{ 0.5f,  0.5f, -0.5f,   0.0f,  n_xy,  n_z},
        MeshV_PN{-0.5f,  0.5f, -0.5f,   0.0f,  n_xy,  n_z},
        MeshV_PN{ 0.0f,  0.0f,  0.5f,   0.0f,  n_xy,  n_z},

        // -X face
        MeshV_PN{-0.5f,  0.5f, -0.5f,  -n_xy,  0.0f,  n_z},
        MeshV_PN{-0.5f, -0.5f, -0.5f,  -n_xy,  0.0f,  n_z},
        MeshV_PN{ 0.0f,  0.0f,  0.5f,  -n_xy,  0.0f,  n_z},

        // Base face (normal -Z)
        MeshV_PN{-0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f},
        MeshV_PN{ 0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f},
        MeshV_PN{ 0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f},

        MeshV_PN{-0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f},
        MeshV_PN{-0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f},
        MeshV_PN{ 0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f},
    };
    // clang-format on

    return mesh_pn_from_array(verts);
}

auto create_cylinder_mesh(int n_segments, f32 radius, f32 height) -> MeshDataPN
{
    {
        Expects(n_segments >= 3);
        Expects(radius > 0.0f && height > 0.0f);
    }

    const auto r = radius;
    const auto hz = 0.5f * height;

    MeshDataPN out{};
    out.vertices.reserve(static_cast<usize>(n_segments) * 12zu);

    auto push = [&](const glm::vec3& p, const glm::vec3& n) -> void
    { out.vertices.push_back(MeshV_PN{p.x, p.y, p.z, n.x, n.y, n.z}); };

    const auto inv_seg = 1.0f / static_cast<f32>(n_segments);

    for (int i{0}; i < n_segments; ++i)
    {
        const auto i_f = static_cast<f32>(i);
        const auto a0 = (i_f * inv_seg) * k_two_pi;
        const auto a1 = ((i_f + 1.0f) * inv_seg) * k_two_pi;

        const auto c0 = std::cos(a0);
        const auto s0 = std::sin(a0);
        const auto c1 = std::cos(a1);
        const auto s1 = std::sin(a1);

        const glm::vec3 n0{c0, s0, 0.0f};
        const glm::vec3 n1{c1, s1, 0.0f};

        const auto z0 = -hz;
        const auto z1 = hz;

        const glm::vec3 p00{r * c0, r * s0, z0};
        const glm::vec3 p01{r * c0, r * s0, z1};
        const glm::vec3 p10{r * c1, r * s1, z0};
        const glm::vec3 p11{r * c1, r * s1, z1};

        push(p00, n0);
        push(p11, n1);
        push(p01, n0);

        push(p00, n0);
        push(p10, n1);
        push(p11, n1);
    }

    auto emit_cap = [&](f32 z, const glm::vec3& center, const glm::vec3& normal) -> void
    {
        for (int i{0}; i < n_segments; ++i)
        {
            const auto i_f = static_cast<f32>(i);

            const auto a0 = (i_f * inv_seg) * k_two_pi;
            const auto a1 = ((i_f + 1.0f) * inv_seg) * k_two_pi;

            const auto c0 = std::cos(a0);
            const auto s0 = std::sin(a0);
            const auto c1 = std::cos(a1);
            const auto s1 = std::sin(a1);

            const glm::vec3 p0{r * c0, r * s0, z};
            const glm::vec3 p1{r * c1, r * s1, z};

            push(center, normal);
            if (normal.z < 0.0f)
            {
                push(p0, normal);
                push(p1, normal);
            }
            else
            {
                push(p1, normal);
                push(p0, normal);
            }
        }
    };

    emit_cap(hz, hz * k_axis_z, k_axis_z);
    emit_cap(-hz, -hz * k_axis_z, k_axis_z);

#ifndef DEBUG
    const auto expected = static_cast<usize>(n_segments) * 12zu;
    Ensures(out.vertices.size() == expected);
#endif
    return out;
}

auto create_sphere_mesh(int lat, int lon, f32 radius) -> MeshDataPN
{
    {
        Expects(lat >= 2 && lon >= 3);
        Expects(radius > 0.0f);
    }

    const auto lat_f = static_cast<f32>(lat);
    const auto lon_f = static_cast<f32>(lon);

    MeshDataPN out{};
    const auto n_verts = static_cast<usize>(lon) * 6zu * static_cast<usize>(lat - 1);
    out.vertices.reserve(n_verts);

    auto push = [&](const Pos3& p, const Dir3& n) -> void
    { out.vertices.push_back(MeshV_PN{p.x, p.y, p.z, n.x, n.y, n.z}); };

    const auto n_top = k_axis_z;
    const auto n_bot = -k_axis_z;
    const auto p_top = radius * n_top;
    const auto p_bot = radius * n_bot;

    auto unit = [&](f32 theta, f32 phi) -> Dir3
    {
        const auto s_theta = std::sin(theta);
        const auto c_theta = std::cos(theta);
        const auto s_phi = std::sin(phi);
        const auto c_phi = std::cos(phi);
        return Dir3{s_theta * c_phi, s_theta * s_phi, c_theta};
    };

    {  // Top
        const auto theta1 = k_pi / lat_f;
        for (int j{0}; j < lon; ++j)
        {
            const auto j_f = static_cast<f32>(j);

            const auto phi0 = (j_f / lon_f) * k_two_pi;
            const auto phi1 = ((j_f + 1.0f) / lon_f) * k_two_pi;

            const auto n10 = unit(theta1, phi0);
            const auto n11 = unit(theta1, phi1);

            const auto p10 = radius * n10;
            const auto p11 = radius * n11;

            push(p_top, n_top);
            push(p10, n10);
            push(p11, n11);
        }
    }

    for (int i{1}; i <= lat - 2; ++i)
    {  // Middle
        const auto i_f = static_cast<f32>(i);
        const auto theta0 = (i_f / lat_f) * k_pi;
        const auto theta1 = ((i_f + 1.0f) / lat_f) * k_pi;

        for (int j{0}; j < lon; ++j)
        {
            const auto j_f = static_cast<f32>(j);

            const auto phi0 = (j_f / lon_f) * k_two_pi;
            const auto phi1 = ((j_f + 1.0f) / lon_f) * k_two_pi;

            const auto n00 = unit(theta0, phi0);
            const auto n10 = unit(theta0, phi1);
            const auto n01 = unit(theta1, phi0);
            const auto n11 = unit(theta1, phi1);

            const auto p00 = radius * n00;
            const auto p10 = radius * n10;
            const auto p01 = radius * n01;
            const auto p11 = radius * n11;

            push(p00, n00);
            push(p01, n01);
            push(p11, n11);

            push(p00, n00);
            push(p11, n11);
            push(p10, n10);
        }
    }

    {  // Bottom
        const auto theta0 = (static_cast<f32>(lat - 1) / static_cast<f32>(lat)) * k_pi;
        for (int j{0}; j < lon; ++j)
        {
            const auto j_f = static_cast<f32>(j);

            const auto phi0 = (j_f / lon_f) * k_two_pi;
            const auto phi1 = ((j_f + 1.0f) / lon_f) * k_two_pi;

            const auto n00 = unit(theta0, phi0);
            const auto n01 = unit(theta0, phi1);
            const auto p00 = radius * n00;
            const auto p01 = radius * n01;

            push(p00, n00);
            push(p_bot, n_bot);
            push(p01, n01);
        }
    }

#ifndef NDEBUG
    const usize expected{static_cast<usize>(lon) * 6zu * static_cast<usize>(lat - 1)};
    Ensures(out.vertices.size() == expected);
#endif
    return out;
}

auto create_grid_mesh(GridSettings grid) -> MeshDataPColor
{
    {
        Expects(grid.n_lines_per_side >= 1 && "GridSettings.n_lines_per_side must be >= 1");
        Expects(grid.spacing > 0.0f && "GridSettings.spacing must be > 0");
        Expects(in_interval(grid.minor_alpha, 0.0f, 1.0f));
        Expects(in_interval(grid.axis_alpha, 0.0f, 1.0f));
    }

    const auto n_lines_per_side = grid.n_lines_per_side;
    const auto he = static_cast<f32>(n_lines_per_side) * grid.spacing;

    MeshDataPColor out{};
    out.vertices.reserve(static_cast<usize>((2 * n_lines_per_side + 1) * 4));

    auto push_line = [&](Pos3 a, Pos3 b, f32 r, f32 g, f32 bl, f32 al) -> void
    {
        out.vertices.push_back(MeshV_PColor{a.x, a.y, a.z, r, g, bl, al});
        out.vertices.push_back(MeshV_PColor{b.x, b.y, b.z, r, g, bl, al});
    };

    // x = const -> y axis parallels
    for (auto i = -n_lines_per_side; i <= n_lines_per_side; ++i)
    {
        const auto x = static_cast<f32>(i) * grid.spacing;
        if (i == 0)
        {
            push_line({x, -he, 0.0f}, {x, he, 0.0f}, 0.15f, 0.90f, 0.25f, grid.axis_alpha);
        }
        else
        {
            push_line({x, -he, 0.0f}, {x, he, 0.0f}, 0.65f, 0.68f, 0.72f, grid.minor_alpha);
        }
    }

    // y = const -> x axis parallels
    for (auto i = -n_lines_per_side; i <= n_lines_per_side; ++i)
    {
        const auto y = static_cast<f32>(i) * grid.spacing;
        if (i == 0)
        {
            push_line({-he, y, 0.0f}, {he, y, 0.0f}, 0.90f, 0.20f, 0.18f, grid.axis_alpha);
        }
        else
        {
            push_line({-he, y, 0.0f}, {he, y, 0.0f}, 0.65f, 0.68f, 0.72f, grid.minor_alpha);
        }
    }

#ifndef DEBUG
    [[maybe_unused]] const auto expected = static_cast<usize>((2 * n_lines_per_side + 1) * 4);
    Ensures(out.vertices.size() == expected);
#endif
    return out;
}

}  // namespace ds_pba
