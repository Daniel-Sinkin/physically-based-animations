// pba/assets/mesh.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/assets/mesh.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/math_types.hpp"

#include <optional>
#include <print>

namespace ds_pba
{
namespace
{

template <usize N>
[[nodiscard]] MeshDataPN mesh_pn_from_array(const std::array<MeshV_PN, N>& a)
{
    MeshDataPN out{};
    out.vertices.reserve(N);
    out.vertices.insert(out.vertices.end(), a.begin(), a.end());
    return out;
}

[[nodiscard]] bool alpha_in_01(f32 a) noexcept
{
    return (a >= 0.0f) && (a <= 1.0f);
}

}  // namespace

MeshDataPN create_cube_mesh()
{
    // clang-format off
    static constexpr std::array<MeshV_PN, 36> verts = {
        // +Z
        MeshV_PN{-0.5f, -0.5f,  0.5f,  0,  0,  1},
        MeshV_PN{ 0.5f, -0.5f,  0.5f,  0,  0,  1},
        MeshV_PN{ 0.5f,  0.5f,  0.5f,  0,  0,  1},
        MeshV_PN{-0.5f, -0.5f,  0.5f,  0,  0,  1},
        MeshV_PN{ 0.5f,  0.5f,  0.5f,  0,  0,  1},
        MeshV_PN{-0.5f,  0.5f,  0.5f,  0,  0,  1},

        // -Z
        MeshV_PN{-0.5f, -0.5f, -0.5f,  0,  0, -1},
        MeshV_PN{ 0.5f,  0.5f, -0.5f,  0,  0, -1},
        MeshV_PN{ 0.5f, -0.5f, -0.5f,  0,  0, -1},
        MeshV_PN{-0.5f, -0.5f, -0.5f,  0,  0, -1},
        MeshV_PN{-0.5f,  0.5f, -0.5f,  0,  0, -1},
        MeshV_PN{ 0.5f,  0.5f, -0.5f,  0,  0, -1},

        // +X
        MeshV_PN{ 0.5f, -0.5f, -0.5f,  1,  0,  0},
        MeshV_PN{ 0.5f,  0.5f, -0.5f,  1,  0,  0},
        MeshV_PN{ 0.5f,  0.5f,  0.5f,  1,  0,  0},
        MeshV_PN{ 0.5f, -0.5f, -0.5f,  1,  0,  0},
        MeshV_PN{ 0.5f,  0.5f,  0.5f,  1,  0,  0},
        MeshV_PN{ 0.5f, -0.5f,  0.5f,  1,  0,  0},

        // -X
        MeshV_PN{-0.5f, -0.5f, -0.5f, -1,  0,  0},
        MeshV_PN{-0.5f,  0.5f,  0.5f, -1,  0,  0},
        MeshV_PN{-0.5f,  0.5f, -0.5f, -1,  0,  0},
        MeshV_PN{-0.5f, -0.5f, -0.5f, -1,  0,  0},
        MeshV_PN{-0.5f, -0.5f,  0.5f, -1,  0,  0},
        MeshV_PN{-0.5f,  0.5f,  0.5f, -1,  0,  0},

        // +Y
        MeshV_PN{-0.5f,  0.5f, -0.5f,  0,  1,  0},
        MeshV_PN{ 0.5f,  0.5f,  0.5f,  0,  1,  0},
        MeshV_PN{ 0.5f,  0.5f, -0.5f,  0,  1,  0},
        MeshV_PN{-0.5f,  0.5f, -0.5f,  0,  1,  0},
        MeshV_PN{-0.5f,  0.5f,  0.5f,  0,  1,  0},
        MeshV_PN{ 0.5f,  0.5f,  0.5f,  0,  1,  0},

        // -Y
        MeshV_PN{-0.5f, -0.5f, -0.5f,  0, -1,  0},
        MeshV_PN{ 0.5f, -0.5f, -0.5f,  0, -1,  0},
        MeshV_PN{ 0.5f, -0.5f,  0.5f,  0, -1,  0},
        MeshV_PN{-0.5f, -0.5f, -0.5f,  0, -1,  0},
        MeshV_PN{ 0.5f, -0.5f,  0.5f,  0, -1,  0},
        MeshV_PN{-0.5f, -0.5f,  0.5f,  0, -1,  0},
    };
    // clang-format on

    return mesh_pn_from_array(verts);
}

MeshDataPN create_quad_mesh()
{
    // clang-format off
    static constexpr std::array<MeshV_PN, 6> verts = {
        MeshV_PN{-0.5f, -0.5f,  0.0f,  0,  0,  1},
        MeshV_PN{ 0.5f, -0.5f,  0.0f,  0,  0,  1},
        MeshV_PN{ 0.5f,  0.5f,  0.0f,  0,  0,  1},
        MeshV_PN{-0.5f, -0.5f,  0.0f,  0,  0,  1},
        MeshV_PN{ 0.5f,  0.5f,  0.0f,  0,  0,  1},
        MeshV_PN{-0.5f,  0.5f,  0.0f,  0,  0,  1},
    };
    // clang-format on

    return mesh_pn_from_array(verts);
}

MeshDataPN create_pyramid_mesh()
{
    static constexpr f32 n_xy{0.8944271909999159f};  // 2 / sqrt(5)
    static constexpr f32 n_z{0.4472135954999579f};   // 1 / sqrt(5)

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

std::optional<MeshDataPN> create_cylinder_mesh(int n_segments, f32 radius, f32 height)
{
    if (n_segments < 3)
    {
        std::println(
            stderr,
            "[Warning] create_cylinder_mesh: invalid n_segments={} (expected >= 3)",
            n_segments
        );
        return std::nullopt;
    }
    if (!(radius > 0.0f) || !(height > 0.0f))
    {
        std::println(
            stderr,
            "[Warning] create_cylinder_mesh: invalid radius/height (radius={}, height={})",
            static_cast<f64>(radius),
            static_cast<f64>(height)
        );
        return std::nullopt;
    }

    const f32 r{radius};
    const f32 hz{0.5f * height};

    MeshDataPN out{};
    out.vertices.reserve(static_cast<usize>(n_segments) * 12zu);

    auto push = [&](const glm::vec3& p, const glm::vec3& n) -> void
    { out.vertices.push_back(MeshV_PN{p.x, p.y, p.z, n.x, n.y, n.z}); };

    const f32 inv_seg{1.0f / static_cast<f32>(n_segments)};

    for (int i{0}; i < n_segments; ++i)
    {
        const f32 i_f{static_cast<f32>(i)};
        const f32 a0{(i_f * inv_seg) * k_two_pi};
        const f32 a1{((i_f + 1.0f) * inv_seg) * k_two_pi};

        const f32 c0{std::cos(a0)};
        const f32 s0{std::sin(a0)};
        const f32 c1{std::cos(a1)};
        const f32 s1{std::sin(a1)};

        const glm::vec3 n0{c0, s0, 0.0f};
        const glm::vec3 n1{c1, s1, 0.0f};

        const f32 z0{-hz};
        const f32 z1{hz};

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

    // Caps
    const glm::vec3 n_top{k_axis_z};
    const glm::vec3 n_bot{-k_axis_z};

    const glm::vec3 c_top{0.0f, 0.0f, hz};
    const glm::vec3 c_bot{0.0f, 0.0f, -hz};

    auto emit_cap = [&](f32 z, const glm::vec3& center, const glm::vec3& normal) -> void
    {
        for (int i{0}; i < n_segments; ++i)
        {
            const f32 i_f{static_cast<f32>(i)};

            const f32 a0{(i_f * inv_seg) * k_two_pi};
            const f32 a1{((i_f + 1.0f) * inv_seg) * k_two_pi};

            const f32 c0{std::cos(a0)};
            const f32 s0{std::sin(a0)};
            const f32 c1{std::cos(a1)};
            const f32 s1{std::sin(a1)};

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

    emit_cap(hz, c_top, n_top);
    emit_cap(-hz, c_bot, n_bot);

    const usize expected{static_cast<usize>(n_segments) * 12zu};
    const usize actual{out.vertices.size()};
    if (actual != expected)
    {
        std::println(
            stderr,
            "[Warning] create_cylinder_mesh: vertex count mismatch (n_segments={}, radius={}, "
            "height={}, expected={}, got={})",
            n_segments,
            static_cast<f64>(radius),
            static_cast<f64>(height),
            expected,
            actual
        );
        return std::nullopt;
    }

    return out;
}

std::optional<MeshDataPN> create_sphere_mesh(int lat, int lon, f32 radius)
{
    if (lat < 2 || lon < 3)
    {
        std::println(
            stderr,
            "[Warning] create_sphere_mesh: invalid lat/lon (lat={}, lon={}, expected lat>=2 "
            "lon>=3)",
            lat,
            lon
        );
        return std::nullopt;
    }
    if (!(radius > 0.0f))
    {
        std::println(
            stderr,
            "[Warning] create_sphere_mesh: invalid radius={} (expected > 0)",
            static_cast<f64>(radius)
        );
        return std::nullopt;
    }

    const f32 lat_f{static_cast<f32>(lat)};
    const f32 lon_f{static_cast<f32>(lon)};

    MeshDataPN out{};
    out.vertices.reserve(static_cast<usize>(lon) * 6zu * static_cast<usize>(lat));

    auto push = [&](const Position3& p, const Direction3& n) -> void
    { out.vertices.push_back(MeshV_PN{p.x, p.y, p.z, n.x, n.y, n.z}); };

    const Direction3 n_top{0.0f, 0.0f, 1.0f};
    const Direction3 n_bot{0.0f, 0.0f, -1.0f};
    const Position3 p_top{radius * n_top};
    const Position3 p_bot{radius * n_bot};

    auto unit = [&](f32 theta, f32 phi) -> Direction3
    {
        return Direction3{
            std::sin(theta) * std::cos(phi),
            std::sin(theta) * std::sin(phi),
            std::cos(theta),
        };
    };

    // Top cap
    {
        const f32 theta1{k_pi / lat_f};
        for (int j{0}; j < lon; ++j)
        {
            const f32 j_f{static_cast<f32>(j)};

            const f32 phi0{(j_f / lon_f) * k_two_pi};
            const f32 phi1{((j_f + 1.0f) / lon_f) * k_two_pi};

            const Direction3 n10{unit(theta1, phi0)};
            const Direction3 n11{unit(theta1, phi1)};

            const Position3 p10{radius * n10};
            const Position3 p11{radius * n11};

            push(p_top, n_top);
            push(p10, n10);
            push(p11, n11);
        }
    }

    // Middle quads
    for (int i{1}; i <= lat - 2; ++i)
    {
        const f32 i_f{static_cast<f32>(i)};
        const f32 theta0{(i_f / lat_f) * k_pi};
        const f32 theta1{((i_f + 1.0f) / lat_f) * k_pi};

        for (int j{0}; j < lon; ++j)
        {
            const f32 j_f{static_cast<f32>(j)};

            const f32 phi0{(j_f / lon_f) * k_two_pi};
            const f32 phi1{((j_f + 1.0f) / lon_f) * k_two_pi};

            const Direction3 n00{unit(theta0, phi0)};
            const Direction3 n10{unit(theta0, phi1)};
            const Direction3 n01{unit(theta1, phi0)};
            const Direction3 n11{unit(theta1, phi1)};

            const Position3 p00{radius * n00};
            const Position3 p10{radius * n10};
            const Position3 p01{radius * n01};
            const Position3 p11{radius * n11};

            push(p00, n00);
            push(p01, n01);
            push(p11, n11);

            push(p00, n00);
            push(p11, n11);
            push(p10, n10);
        }
    }

    // Bottom cap
    {
        const f32 theta0{(static_cast<f32>(lat - 1) / static_cast<f32>(lat)) * k_pi};
        for (int j{0}; j < lon; ++j)
        {
            const f32 j_f{static_cast<f32>(j)};

            const f32 phi0{(j_f / lon_f) * k_two_pi};
            const f32 phi1{((j_f + 1.0f) / lon_f) * k_two_pi};

            const Direction3 n00{unit(theta0, phi0)};
            const Direction3 n01{unit(theta0, phi1)};

            const Position3 p00{radius * n00};
            const Position3 p01{radius * n01};

            push(p00, n00);
            push(p_bot, n_bot);
            push(p01, n01);
        }
    }

    const usize expected{static_cast<usize>(lon) * 6zu * static_cast<usize>(lat - 1)};
    const usize actual{out.vertices.size()};
    if (actual != expected)
    {
        std::println(
            stderr,
            "[Warning] create_sphere_mesh: vertex count mismatch (lat={}, lon={}, radius={}, "
            "expected={}, got={})",
            lat,
            lon,
            static_cast<f64>(radius),
            expected,
            actual
        );
        return std::nullopt;
    }

    return out;
}

std::optional<MeshDataPColor> create_grid_mesh(GridSettings grid)
{
    if (grid.n_lines_per_side < 1)
    {
        std::println(
            stderr,
            "[Warning] create_grid_mesh: invalid n_lines_per_side={} (expected >= 1)",
            grid.n_lines_per_side
        );
        return std::nullopt;
    }
    if (!(grid.spacing > 0.0f))
    {
        std::println(
            stderr,
            "[Warning] create_grid_mesh: invalid spacing={} (expected > 0)",
            static_cast<f64>(grid.spacing)
        );
        return std::nullopt;
    }
    if (!alpha_in_01(grid.minor_alpha) || !alpha_in_01(grid.axis_alpha))
    {
        std::println(
            stderr,
            "[Warning] create_grid_mesh: invalid alpha(s) (minor_alpha={}, axis_alpha={})",
            static_cast<f64>(grid.minor_alpha),
            static_cast<f64>(grid.axis_alpha)
        );
        return std::nullopt;
    }

    const int N{grid.n_lines_per_side};
    const f32 E{static_cast<f32>(N) * grid.spacing};

    MeshDataPColor out{};
    out.vertices.reserve(static_cast<usize>((2 * N + 1) * 4));

    auto push_line = [&](Position3 a, Position3 b, f32 r, f32 g, f32 bl, f32 al) -> void
    {
        out.vertices.push_back(MeshV_PColor{a.x, a.y, a.z, r, g, bl, al});
        out.vertices.push_back(MeshV_PColor{b.x, b.y, b.z, r, g, bl, al});
    };

    // x = const -> y axis parallels
    for (int i{-N}; i <= N; ++i)
    {
        const f32 x{static_cast<f32>(i) * grid.spacing};
        if (i == 0)
        {
            push_line({x, -E, 0.0f}, {x, E, 0.0f}, 0.15f, 0.90f, 0.25f, grid.axis_alpha);
        }
        else
        {
            push_line({x, -E, 0.0f}, {x, E, 0.0f}, 0.65f, 0.68f, 0.72f, grid.minor_alpha);
        }
    }

    // y = const -> x axis parallels
    for (int i{-N}; i <= N; ++i)
    {
        const f32 y{static_cast<f32>(i) * grid.spacing};
        if (i == 0)
        {
            push_line({-E, y, 0.0f}, {E, y, 0.0f}, 0.90f, 0.20f, 0.18f, grid.axis_alpha);
        }
        else
        {
            push_line({-E, y, 0.0f}, {E, y, 0.0f}, 0.65f, 0.68f, 0.72f, grid.minor_alpha);
        }
    }

    const usize expected{static_cast<usize>((2 * N + 1) * 4)};
    const usize actual{out.vertices.size()};
    if (actual != expected)
    {
        std::println(
            stderr,
            "[Warning] create_grid_mesh: vertex count mismatch (N={}, expected={}, got={})",
            N,
            expected,
            actual
        );
        return std::nullopt;
    }

    return out;
}

}  // namespace ds_pba
