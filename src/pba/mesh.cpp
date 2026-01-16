// pba/mesh.cpp
#include "mesh.hpp"
#include "pba/types.hpp" // IWYU pragma: keep

#include <algorithm>
#include <array>
#include <vector>

namespace ds_pba {

GLMesh create_cube_mesh() {
    struct V {
        f32 px, py, pz;
        f32 nx, ny, nz;
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V),GLPtr::offset0());

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset(3 * sizeof(f32)));

    VBO::unbind();
    VAO::unbind();
    return mesh;
}

GLMesh create_grid_mesh(int n_lines_per_side, f32 spacing, f32 axis_alpha, f32 minor_alpha) {
    struct V {
        f32 px, py, pz;
        f32 r, g, b, a;
    };

    const int N = std::max(1, n_lines_per_side);
    const f32 E = static_cast<f32>(N) * spacing;

    std::vector<V> verts;
    verts.reserve(static_cast<usize>((2 * N + 1) * 4));

    auto push_line = [&](glm::vec3 a, glm::vec3 b, f32 r, f32 g, f32 bl, f32 al) {
        verts.push_back(V{a.x, a.y, a.z, r, g, bl, al});
        verts.push_back(V{b.x, b.y, b.z, r, g, bl, al});
    };

    // Lines parallel to Y axis (x = const)
    for (int i = -N; i <= N; ++i) {
        f32 x = static_cast<f32>(i) * spacing;
        if (i == 0) {
            // y-axis: x=0 -> green
            push_line({x, -E, 0}, {x, E, 0}, 0.15f, 0.90f, 0.25f, axis_alpha);
        } else {
            push_line({x, -E, 0}, {x, E, 0}, 0.65f, 0.68f, 0.72f, minor_alpha);
        }
    }

    // Lines parallel to X axis (y = const)
    for (int i = -N; i <= N; ++i) {
        f32 y = static_cast<f32>(i) * spacing;
        if (i == 0) {
            // x-axis: y=0 -> red
            push_line({-E, y, 0}, {E, y, 0}, 0.90f, 0.20f, 0.18f, axis_alpha);
        } else {
            push_line({-E, y, 0}, {E, y, 0}, 0.65f, 0.68f, 0.72f, minor_alpha);
        }
    }

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    mesh.vao.bind();
    mesh.vbo.bind();
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(V)), verts.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset0());

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(V), GLPtr::offset(3 * sizeof(f32)));

    VBO::unbind();
    VAO::unbind();
    return mesh;
}

} // namespace ds_pba