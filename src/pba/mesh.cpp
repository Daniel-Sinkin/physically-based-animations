// pba/mesh.cpp
#include "mesh.hpp"

#include "pba/types.hpp"  // IWYU pragma: keep

#include <algorithm>
#include <array>
#include <numbers>
#include <vector>

namespace ds_pba
{

GLMesh create_cube_mesh()
{
    struct V
    {
        f32 px, py, pz;  // points
        f32 nx, ny, nz;  // normals
    };

    static constexpr std::array<V, 36> verts = {
        // +Z
        V{-0.5f, -0.5f, 0.5f, 0, 0, 1},
        V{0.5f, -0.5f, 0.5f, 0, 0, 1},
        V{0.5f, 0.5f, 0.5f, 0, 0, 1},
        V{-0.5f, -0.5f, 0.5f, 0, 0, 1},
        V{0.5f, 0.5f, 0.5f, 0, 0, 1},
        V{-0.5f, 0.5f, 0.5f, 0, 0, 1},
        // -Z
        V{-0.5f, -0.5f, -0.5f, 0, 0, -1},
        V{0.5f, 0.5f, -0.5f, 0, 0, -1},
        V{0.5f, -0.5f, -0.5f, 0, 0, -1},
        V{-0.5f, -0.5f, -0.5f, 0, 0, -1},
        V{-0.5f, 0.5f, -0.5f, 0, 0, -1},
        V{0.5f, 0.5f, -0.5f, 0, 0, -1},
        // +X
        V{0.5f, -0.5f, -0.5f, 1, 0, 0},
        V{0.5f, 0.5f, -0.5f, 1, 0, 0},
        V{0.5f, 0.5f, 0.5f, 1, 0, 0},
        V{0.5f, -0.5f, -0.5f, 1, 0, 0},
        V{0.5f, 0.5f, 0.5f, 1, 0, 0},
        V{0.5f, -0.5f, 0.5f, 1, 0, 0},
        // -X
        V{-0.5f, -0.5f, -0.5f, -1, 0, 0},
        V{-0.5f, 0.5f, 0.5f, -1, 0, 0},
        V{-0.5f, 0.5f, -0.5f, -1, 0, 0},
        V{-0.5f, -0.5f, -0.5f, -1, 0, 0},
        V{-0.5f, -0.5f, 0.5f, -1, 0, 0},
        V{-0.5f, 0.5f, 0.5f, -1, 0, 0},
        // +Y
        V{-0.5f, 0.5f, -0.5f, 0, 1, 0},
        V{0.5f, 0.5f, 0.5f, 0, 1, 0},
        V{0.5f, 0.5f, -0.5f, 0, 1, 0},
        V{-0.5f, 0.5f, -0.5f, 0, 1, 0},
        V{-0.5f, 0.5f, 0.5f, 0, 1, 0},
        V{0.5f, 0.5f, 0.5f, 0, 1, 0},
        // -Y
        V{-0.5f, -0.5f, -0.5f, 0, -1, 0},
        V{0.5f, -0.5f, -0.5f, 0, -1, 0},
        V{0.5f, -0.5f, 0.5f, 0, -1, 0},
        V{-0.5f, -0.5f, -0.5f, 0, -1, 0},
        V{0.5f, -0.5f, 0.5f, 0, -1, 0},
        V{-0.5f, -0.5f, 0.5f, 0, -1, 0},
    };

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    mesh.vao.bind();
    mesh.vbo.bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset0());

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset(3 * sizeof(f32)));

    VBO::unbind();
    VAO::unbind();
    return mesh;
}

GLMesh create_grid_mesh(int n_lines_per_side, f32 spacing, f32 axis_alpha, f32 minor_alpha)
{
    struct V
    {
        f32 px, py, pz;
        f32 r, g, b, a;
    };

    const int N = std::max(1, n_lines_per_side);
    const f32 E = static_cast<f32>(N) * spacing;

    std::vector<V> verts;
    verts.reserve(static_cast<usize>((2 * N + 1) * 4));

    auto push_line = [&](glm::vec3 a, glm::vec3 b, f32 r, f32 g, f32 bl, f32 al)
    {
        verts.push_back(V{a.x, a.y, a.z, r, g, bl, al});
        verts.push_back(V{b.x, b.y, b.z, r, g, bl, al});
    };

    // x = const -> y axis parallels
    for (int i = -N; i <= N; ++i)
    {
        f32 x = static_cast<f32>(i) * spacing;
        if (i == 0)
        {
            push_line({x, -E, 0}, {x, E, 0}, 0.15f, 0.90f, 0.25f, axis_alpha);
        }
        else
        {
            push_line({x, -E, 0}, {x, E, 0}, 0.65f, 0.68f, 0.72f, minor_alpha);
        }
    }

    // y = const -> x axis parallels
    for (int i = -N; i <= N; ++i)
    {
        f32 y = static_cast<f32>(i) * spacing;
        if (i == 0)
        {
            push_line({-E, y, 0}, {E, y, 0}, 0.90f, 0.20f, 0.18f, axis_alpha);
        }
        else
        {
            push_line({-E, y, 0}, {E, y, 0}, 0.65f, 0.68f, 0.72f, minor_alpha);
        }
    }

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    mesh.vao.bind();
    mesh.vbo.bind();
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(verts.size() * sizeof(V)),
        verts.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset0());

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset(3 * sizeof(f32)));

    VBO::unbind();
    VAO::unbind();
    return mesh;
}

GLMesh create_sphere_mesh(int n_lat, int n_lon, f32 radius)
{
    struct V
    {
        f32 px, py, pz;
        f32 nx, ny, nz;
    };

    const int lat = std::max(2, n_lat);
    const int lon = std::max(3, n_lon);

    static constexpr f32 k_pi = std::numbers::pi_v<f32>;
    static constexpr f32 k_two_pi = 2.0f * std::numbers::pi_v<f32>;

    std::vector<V> verts;
    verts.reserve(static_cast<usize>(lon) * 6zu * static_cast<usize>(lat));

    auto push = [&](const glm::vec3& p, const glm::vec3& n)
    { verts.push_back(V{p.x, p.y, p.z, n.x, n.y, n.z}); };

    const glm::vec3 n_top{0.0f, 0.0f, 1.0f};
    const glm::vec3 n_bot{0.0f, 0.0f, -1.0f};
    const glm::vec3 p_top = radius * n_top;
    const glm::vec3 p_bot = radius * n_bot;

    auto unit = [&](f32 theta, f32 phi) -> glm::vec3
    {
        const f32 st = std::sin(theta);
        const f32 ct = std::cos(theta);
        const f32 sp = std::sin(phi);
        const f32 cp = std::cos(phi);
        return glm::vec3{st * cp, st * sp, ct};
    };

    {
        const f32 theta1 = k_pi / static_cast<f32>(lat);
        for (int j = 0; j < lon; ++j)
        {
            const f32 phi0 = (static_cast<f32>(j) / static_cast<f32>(lon)) * k_two_pi;
            const f32 phi1 = (static_cast<f32>(j + 1) / static_cast<f32>(lon)) * k_two_pi;

            const glm::vec3 n10 = unit(theta1, phi0);
            const glm::vec3 n11 = unit(theta1, phi1);

            const glm::vec3 p10 = radius * n10;
            const glm::vec3 p11 = radius * n11;

            push(p_top, n_top);
            push(p10, n10);
            push(p11, n11);
        }
    }

    for (int i = 1; i <= lat - 2; ++i)
    {
        const f32 theta0 = (static_cast<f32>(i) / static_cast<f32>(lat)) * k_pi;
        const f32 theta1 = (static_cast<f32>(i + 1) / static_cast<f32>(lat)) * k_pi;

        for (int j = 0; j < lon; ++j)
        {
            const f32 phi0 = (static_cast<f32>(j) / static_cast<f32>(lon)) * k_two_pi;
            const f32 phi1 = (static_cast<f32>(j + 1) / static_cast<f32>(lon)) * k_two_pi;

            const glm::vec3 n00 = unit(theta0, phi0);
            const glm::vec3 n10 = unit(theta0, phi1);
            const glm::vec3 n01 = unit(theta1, phi0);
            const glm::vec3 n11 = unit(theta1, phi1);

            const glm::vec3 p00 = radius * n00;
            const glm::vec3 p10 = radius * n10;
            const glm::vec3 p01 = radius * n01;
            const glm::vec3 p11 = radius * n11;

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
        for (int j = 0; j < lon; ++j)
        {
            const f32 phi0 = (static_cast<f32>(j) / static_cast<f32>(lon)) * k_two_pi;
            const f32 phi1 = (static_cast<f32>(j + 1) / static_cast<f32>(lon)) * k_two_pi;

            const glm::vec3 n00 = unit(theta0, phi0);
            const glm::vec3 n01 = unit(theta0, phi1);

            const glm::vec3 p00 = radius * n00;
            const glm::vec3 p01 = radius * n01;

            push(p00, n00);
            push(p_bot, n_bot);
            push(p01, n01);
        }
    }

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    mesh.vao.bind();
    mesh.vbo.bind();
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

    VBO::unbind();
    VAO::unbind();
    return mesh;
}

}  // namespace ds_pba
