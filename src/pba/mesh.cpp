// pba/mesh.cpp
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/mesh.hpp"
//
#include "pba/gl_types.hpp"
#include "pba/math_types.hpp"
#include "pba/render_context.hpp"

namespace ds_pba
{
namespace
{
struct V
{
    f32 px, py, pz;  // points
    f32 nx, ny, nz;  // normals
};
}  // namespace

GLMesh create_cube_mesh()
{

    // clang-format off
    static constexpr std::array<V, 36> verts = {
        // +Z
        V{-0.5f, -0.5f,  0.5f,  0,  0,  1},
        V{ 0.5f, -0.5f,  0.5f,  0,  0,  1},
        V{ 0.5f,  0.5f,  0.5f,  0,  0,  1},
        V{-0.5f, -0.5f,  0.5f,  0,  0,  1},
        V{ 0.5f,  0.5f,  0.5f,  0,  0,  1},
        V{-0.5f,  0.5f,  0.5f,  0,  0,  1},
        // -Z
        V{-0.5f, -0.5f, -0.5f,  0,  0, -1},
        V{ 0.5f,  0.5f, -0.5f,  0,  0, -1},
        V{ 0.5f, -0.5f, -0.5f,  0,  0, -1},
        V{-0.5f, -0.5f, -0.5f,  0,  0, -1},
        V{-0.5f,  0.5f, -0.5f,  0,  0, -1},
        V{ 0.5f,  0.5f, -0.5f,  0,  0, -1},
        // +X
        V{ 0.5f, -0.5f, -0.5f,  1,  0,  0},
        V{ 0.5f,  0.5f, -0.5f,  1,  0,  0},
        V{ 0.5f,  0.5f,  0.5f,  1,  0,  0},
        V{ 0.5f, -0.5f, -0.5f,  1,  0,  0},
        V{ 0.5f,  0.5f,  0.5f,  1,  0,  0},
        V{ 0.5f, -0.5f,  0.5f,  1,  0,  0},
        // -X
        V{-0.5f, -0.5f, -0.5f, -1,  0,  0},
        V{-0.5f,  0.5f,  0.5f, -1,  0,  0},
        V{-0.5f,  0.5f, -0.5f, -1,  0,  0},
        V{-0.5f, -0.5f, -0.5f, -1,  0,  0},
        V{-0.5f, -0.5f,  0.5f, -1,  0,  0},
        V{-0.5f,  0.5f,  0.5f, -1,  0,  0},
        // +Y
        V{-0.5f,  0.5f, -0.5f,  0,  1,  0},
        V{ 0.5f,  0.5f,  0.5f,  0,  1,  0},
        V{ 0.5f,  0.5f, -0.5f,  0,  1,  0},
        V{-0.5f,  0.5f, -0.5f,  0,  1,  0},
        V{-0.5f,  0.5f,  0.5f,  0,  1,  0},
        V{ 0.5f,  0.5f,  0.5f,  0,  1,  0},
        // -Y
        V{-0.5f, -0.5f, -0.5f,  0, -1,  0},
        V{ 0.5f, -0.5f, -0.5f,  0, -1,  0},
        V{ 0.5f, -0.5f,  0.5f,  0, -1,  0},
        V{-0.5f, -0.5f, -0.5f,  0, -1,  0},
        V{ 0.5f, -0.5f,  0.5f,  0, -1,  0},
        V{-0.5f, -0.5f,  0.5f,  0, -1,  0},
    };
    // clang-format on

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    {
        const ScopedBufferBinder binder{mesh};
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset0());

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset(3 * sizeof(f32)));
    }
    return mesh;
}

GLMesh create_quad_mesh()
{
    // clang-format off
    static constexpr std::array<V, 6> verts = {
        V{-0.5f, -0.5f,  0.0f,  0,  0,  1},
        V{ 0.5f, -0.5f,  0.0f,  0,  0,  1},
        V{ 0.5f,  0.5f,  0.0f,  0,  0,  1},
        V{-0.5f, -0.5f,  0.0f,  0,  0,  1},
        V{ 0.5f,  0.5f,  0.0f,  0,  0,  1},
        V{-0.5f,  0.5f,  0.0f,  0,  0,  1},
    };
    // clang-format on

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    {
        const ScopedBufferBinder binder{mesh};
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset0());

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset(3 * sizeof(f32)));
    }
    return mesh;
}

GLMesh create_pyramid_mesh()
{
    // Square base pyramid:
    //   base: z = -0.5
    //   apex: z = +0.5
    //
    // 5 faces total: 4 side triangles + 1 base quad (2 triangles)

    // clang-format off
    static constexpr std::array<V, 18> verts = {
        // Side faces
        // -Y face
        V{-0.5f, -0.5f, -0.5f,  0.0f, -0.89442719f,  0.44721360f},
        V{ 0.5f, -0.5f, -0.5f,  0.0f, -0.89442719f,  0.44721360f},
        V{ 0.0f,  0.0f,  0.5f,  0.0f, -0.89442719f,  0.44721360f},

        // +X face
        V{ 0.5f, -0.5f, -0.5f,  0.89442719f,  0.0f,  0.44721360f},
        V{ 0.5f,  0.5f, -0.5f,  0.89442719f,  0.0f,  0.44721360f},
        V{ 0.0f,  0.0f,  0.5f,  0.89442719f,  0.0f,  0.44721360f},

        // +Y face
        V{ 0.5f,  0.5f, -0.5f,  0.0f,  0.89442719f,  0.44721360f},
        V{-0.5f,  0.5f, -0.5f,  0.0f,  0.89442719f,  0.44721360f},
        V{ 0.0f,  0.0f,  0.5f,  0.0f,  0.89442719f,  0.44721360f},

        // -X face
        V{-0.5f,  0.5f, -0.5f, -0.89442719f,  0.0f,  0.44721360f},
        V{-0.5f, -0.5f, -0.5f, -0.89442719f,  0.0f,  0.44721360f},
        V{ 0.0f,  0.0f,  0.5f, -0.89442719f,  0.0f,  0.44721360f},

        // Base face (normal -Z)
        V{-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f},
        V{ 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f},
        V{ 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f},

        V{-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f},
        V{-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f},
        V{ 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f},
    };
    // clang-format on

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    {
        const ScopedBufferBinder binder{mesh};
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset0());

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset(3 * sizeof(f32)));
    }
    return mesh;
}

GLMesh create_cylinder_mesh(int n_segments, f32 radius, f32 height)
{
    const auto seg_f = static_cast<f32>(n_segments);
    const f32 r = radius;
    const f32 hz = 0.5f * height;

    std::vector<V> verts;
    verts.reserve(static_cast<usize>(n_segments) * 12zu);

    auto push = [&](const glm::vec3& p, const glm::vec3& n)
    { verts.emplace_back(p.x, p.y, p.z, n.x, n.y, n.z); };

    static constexpr f32 k_two_pi = 2.0f * std::numbers::pi_v<f32>;

    for (int i = 0; i < n_segments; ++i)
    {
        const auto i_f = static_cast<f32>(i);
        const f32 a0 = (i_f / seg_f) * k_two_pi;
        const f32 a1 = ((i_f + 1.0f) / seg_f) * k_two_pi;

        const f32 c0 = std::cos(a0);
        const f32 s0 = std::sin(a0);
        const f32 c1 = std::cos(a1);
        const f32 s1 = std::sin(a1);

        const glm::vec3 n0{c0, s0, 0.0f};
        const glm::vec3 n1{c1, s1, 0.0f};

        const glm::vec3 p00{r * c0, r * s0, -hz};
        const glm::vec3 p01{r * c0, r * s0, hz};
        const glm::vec3 p10{r * c1, r * s1, -hz};
        const glm::vec3 p11{r * c1, r * s1, hz};

        push(p00, n0);
        push(p11, n1);
        push(p01, n0);

        push(p00, n0);
        push(p10, n1);
        push(p11, n1);
    }

    // Caps
    const glm::vec3 n_top{0.0f, 0.0f, 1.0f};
    const glm::vec3 n_bot{0.0f, 0.0f, -1.0f};
    const glm::vec3 c_top{0.0f, 0.0f, hz};
    const glm::vec3 c_bot{0.0f, 0.0f, -hz};

    for (int i = 0; i < n_segments; ++i)
    {
        const auto i_f = static_cast<f32>(i);
        const f32 a0 = (i_f / seg_f) * k_two_pi;

        const f32 a1 = ((i_f + 1.0f) / seg_f) * k_two_pi;

        const f32 c0 = std::cos(a0);
        const f32 s0 = std::sin(a0);
        const f32 c1 = std::cos(a1);
        const f32 s1 = std::sin(a1);

        const glm::vec3 p0{r * c0, r * s0, hz};
        const glm::vec3 p1{r * c1, r * s1, hz};

        // Top cap is counter clock wise when looking down (from +Z)
        push(c_top, n_top);
        push(p0, n_top);
        push(p1, n_top);
    }

    for (int i = 0; i < n_segments; ++i)
    {
        const auto i_f = static_cast<f32>(i);
        const f32 a0 = (i_f / seg_f) * k_two_pi;
        const f32 a1 = ((i_f + 1) / seg_f) * k_two_pi;

        const f32 c0 = std::cos(a0);
        const f32 s0 = std::sin(a0);
        const f32 c1 = std::cos(a1);
        const f32 s1 = std::sin(a1);

        const Position3 p0{r * c0, r * s0, -hz};
        const Position3 p1{r * c1, r * s1, -hz};

        // Top cap is counter clock wise when looking down (from +Z)
        push(c_bot, n_bot);
        push(p1, n_bot);
        push(p0, n_bot);
    }

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    {
        const ScopedBufferBinder binder{mesh};
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(verts.size() * sizeof(V)),
            verts.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset0());

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset(3 * sizeof(f32)));
    }
    return mesh;
}

GLMesh create_grid_mesh(GridSettings grid)
{
    struct GridV
    {
        f32 px, py, pz;
        f32 r, g, b, a;
    };

    assert(grid.n_lines_per_side >= 1);
    const int N{grid.n_lines_per_side};

    const f32 E{static_cast<f32>(N) * grid.spacing};

    std::vector<GridV> verts;
    verts.reserve(static_cast<usize>((2 * N + 1) * 4));

    auto push_line = [&](Position3 a, Position3 b, f32 r, f32 g, f32 bl, f32 al)
    {
        verts.emplace_back(a.x, a.y, a.z, r, g, bl, al);
        verts.emplace_back(b.x, b.y, b.z, r, g, bl, al);
    };

    // x = const -> y axis parallels
    for (int i{-N}; i <= N; ++i)
    {
        const f32 x = static_cast<f32>(i) * grid.spacing;
        if (i == 0)
        {
            push_line({x, -E, 0}, {x, E, 0}, 0.15f, 0.90f, 0.25f, grid.axis_alpha);
        }
        else
        {
            push_line({x, -E, 0}, {x, E, 0}, 0.65f, 0.68f, 0.72f, grid.minor_alpha);
        }
    }

    // y = const -> x axis parallels
    for (int i{-N}; i <= N; ++i)
    {
        const f32 y = static_cast<f32>(i) * grid.spacing;
        if (i == 0)
        {
            push_line({-E, y, 0}, {E, y, 0}, 0.90f, 0.20f, 0.18f, grid.axis_alpha);
        }
        else
        {
            push_line({-E, y, 0}, {E, y, 0}, 0.65f, 0.68f, 0.72f, grid.minor_alpha);
        }
    }

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    {
        const ScopedBufferBinder bind{mesh};
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(verts.size() * sizeof(GridV)),
            verts.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GridV), GLPtr::offset0());

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1, 4, GL_FLOAT, GL_FALSE, sizeof(GridV), GLPtr::offset(3 * sizeof(f32))
        );
    }
    return mesh;
}

GLMesh create_sphere_mesh(int lat, int lon, f32 radius)
{
    if (lat < 2 || lon < 3)
    {
        throw std::runtime_error(
            std::format(
                "Expected lat to be >= 2 (actually {}), expected lon to be >= 3 (actually {})",
                lat,
                lon
            )
        );
    }

    const auto lat_f = static_cast<f32>(lat);
    const auto lon_f = static_cast<f32>(lon);

    static constexpr f32 k_pi{std::numbers::pi_v<f32>};
    static constexpr f32 k_two_pi{2.0f * std::numbers::pi_v<f32>};

    std::vector<V> verts;
    verts.reserve(static_cast<usize>(lon) * 6zu * static_cast<usize>(lat));

    auto push = [&](const Position3& p, const Direction3& n)
    { verts.emplace_back(p.x, p.y, p.z, n.x, n.y, n.z); };

    const Direction3 n_top{0.0f, 0.0f, 1.0f};
    const Direction3 n_bot{0.0f, 0.0f, -1.0f};
    const Position3 p_top{radius * n_top};
    const Position3 p_bot{radius * n_bot};

    // Gives the vector at the corresponding (theta, phi) angles
    // which is exactly the normal direction on a sphere
    auto unit = [&](f32 theta, f32 phi) -> Direction3
    {
        return Direction3{
            std::sin(theta) * std::cos(phi), std::sin(theta) * std::sin(phi), std::cos(theta)
        };
    };

    {
        const f32 theta1 = k_pi / lat_f;
        for (int j{0}; j < lon; ++j)
        {
            const auto j_f = static_cast<f32>(j);

            const f32 phi0 = (j_f / lon_f) * k_two_pi;
            const f32 phi1 = ((j_f + 1.0f) / lon_f) * k_two_pi;

            const Direction3 n10{unit(theta1, phi0)};
            const Direction3 n11{unit(theta1, phi1)};

            const Position3 p10{radius * n10};
            const Position3 p11{radius * n11};

            push(p_top, n_top);
            push(p10, n10);
            push(p11, n11);
        }
    }

    for (int i{1}; i <= lat - 2; ++i)
    {
        const auto i_f = static_cast<f32>(i);
        const f32 theta0 = (i_f / lat_f) * k_pi;
        const f32 theta1 = ((i_f + 1.0f) / lat_f) * k_pi;

        for (int j{0}; j < lon; ++j)
        {
            const auto j_f = static_cast<f32>(j);

            const f32 phi0 = (j_f / lon_f) * k_two_pi;
            const f32 phi1 = ((j_f + 1.0f) / lon_f) * k_two_pi;

            const Direction3 n00 = unit(theta0, phi0);
            const Direction3 n10 = unit(theta0, phi1);
            const Direction3 n01 = unit(theta1, phi0);
            const Direction3 n11 = unit(theta1, phi1);

            const Direction3 p00 = radius * n00;
            const Direction3 p10 = radius * n10;
            const Direction3 p01 = radius * n01;
            const Direction3 p11 = radius * n11;

            push(p00, n00);
            push(p01, n01);
            push(p11, n11);

            push(p00, n00);
            push(p11, n11);
            push(p10, n10);
        }
    }

    {
        const f32 theta0 = (static_cast<f32>(lat - 1) / static_cast<f32>(lat)) * k_pi;
        for (int j{0}; j < lon; ++j)
        {
            const auto j_f = static_cast<f32>(j);

            const Direction3 n00{unit(theta0, (j_f * lon_f) / k_two_pi)};
            const Direction3 n01{unit(theta0, ((j_f + 1.0f) + lon_f) / k_two_pi)};

            const Position3 p00{radius * n00};
            const Position3 p01{radius * n01};

            push(p00, n00);
            push(p_bot, n_bot);
            push(p01, n01);
        }
    }

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    {
        const ScopedBufferBinder binder{mesh};
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(verts.size() * sizeof(V)),
            verts.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset0());

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset(3 * sizeof(f32)));
    }
    return mesh;
}

}  // namespace ds_pba
