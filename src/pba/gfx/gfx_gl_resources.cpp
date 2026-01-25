// pba/gfx/gfx_gl_resources.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/assets/gltf_mesh.hpp"
#include "pba/assets/mesh.hpp"
#include "pba/assets/mesh_data.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/gfx/gl.hpp"
#include "pba/gfx/gl_types.hpp"
#include "pba/ui/ui.hpp"
//
#include <imgui.h>
#include <optional>
#include <print>
#include <utility>
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <json.hpp>

namespace ds_pba
{
[[nodiscard]] std::optional<ds_pba::GLMesh>
upload_mesh_pcolor_lines(const ds_pba::MeshDataPColor& mesh_data)
{
    using namespace ds_pba;

    const auto& verts = mesh_data.vertices;
    if (verts.empty())
    {
        std::println(stderr, "Grid mesh data empty!");
        return std::nullopt;
    }

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    {
        const ScopedBufferBinder binder{mesh};

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(verts.size() * sizeof(MeshV_PColor)),
            verts.data(),
            GL_STATIC_DRAW
        );

        const auto stride = static_cast<GLsizei>(sizeof(MeshV_PColor));

        // location 0: position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, GLPtr::offset0());

        // location 1: color
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, GLPtr::offset(3 * sizeof(f32)));
    }

    return mesh;
}
[[nodiscard]] std::optional<ds_pba::GLMesh> upload_mesh_pn(const ds_pba::MeshDataPN& mesh_data)
{
    using namespace ds_pba;

    const auto& verts = mesh_data.vertices;
    if (verts.empty())
    {
        std::println(stderr, "Mesh data empty!");
        return std::nullopt;
    }

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    {
        const ScopedBufferBinder binder{mesh};

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(verts.size() * sizeof(MeshV_PN)),
            verts.data(),
            GL_STATIC_DRAW
        );

        const auto stride = static_cast<GLsizei>(sizeof(MeshV_PN));

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, GLPtr::offset0());

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, GLPtr::offset(3 * sizeof(f32)));
    }
    return mesh;
}
bool GfxContext::create_meshes()
{
    auto upload_or_fail_pn = [&](const std::optional<MeshDataPN>& mesh_data,
                                 std::string_view label) -> std::optional<GLMesh>
    {
        if (!mesh_data)
        {
            std::println(stderr, "Failed to create mesh '{}'", label);
            return std::nullopt;
        }

        auto mesh_res = upload_mesh_pn(*mesh_data);
        if (!mesh_res)
        {
            std::println(stderr, "Failed to upload mesh '{}'", label);
        }
        return mesh_res;
    };

    auto upload_or_fail_pn_value = [&](MeshDataPN mesh_data,
                                       std::string_view label) -> std::optional<GLMesh>
    {
        auto mesh_res = upload_mesh_pn(mesh_data);
        if (!mesh_res)
        {
            std::println(stderr, "Failed to upload mesh '{}'", label);
        }
        return mesh_res;
    };

    auto upload_or_fail_grid = [&](const std::optional<MeshDataPColor>& mesh_data,
                                   std::string_view label) -> std::optional<GLMesh>
    {
        if (!mesh_data)
        {
            std::println(stderr, "Failed to create mesh '{}'", label);
            return std::nullopt;
        }

        auto mesh_res = upload_mesh_pcolor_lines(*mesh_data);
        if (!mesh_res)
        {
            std::println(stderr, "Failed to upload mesh '{}'", label);
        }
        return mesh_res;
    };

    {
        auto mesh_res = upload_or_fail_pn_value(create_cube_mesh(), "cube");
        if (!mesh_res)
        {
            return false;
        }
        cube_mesh = *mesh_res;
    }
    {
        auto mesh_res = upload_or_fail_pn(create_sphere_mesh(32, 24, 1.0f), "sphere");
        if (!mesh_res)
        {
            return false;
        }
        sphere_mesh = *mesh_res;
    }
    {
        auto mesh_res = upload_or_fail_grid(create_grid_mesh(grid), "grid");
        if (!mesh_res)
        {
            return false;
        }
        grid_mesh = *mesh_res;
    }
    {
        auto mesh_res = upload_or_fail_pn(create_cylinder_mesh(24, 0.5f, 1.0f), "cylinder");
        if (!mesh_res)
        {
            return false;
        }
        cylinder_mesh = *mesh_res;
    }
    {
        auto mesh_res = upload_or_fail_pn_value(create_pyramid_mesh(), "pyramid");
        if (!mesh_res)
        {
            return false;
        }
        pyramid_mesh = *mesh_res;
    }
    {
        auto mesh_res = ds_pba::load_model_mesh("marble_bust_01");
        if (!mesh_res)
        {
            std::println(
                stderr,
                "Failed to load marble bust mesh (err={})",
                std::to_underlying(mesh_res.error())
            );
            return false;
        }

        auto m = upload_mesh_pn(*mesh_res);
        if (!m)
        {
            std::println(stderr, "Failed to upload mesh 'marble_bust_01'");
            return false;
        }
        marble_bust_mesh = *m;
    }

    return true;
}

bool GfxContext::create_programs()
{

    {
        auto grid_prog_res = create_program_from_file("grid");
        if (!grid_prog_res)
        {
            std::println(
                stderr,
                "Failed to load 'grid' shaders, got error code: {}",
                std::to_underlying(grid_prog_res.error())
            );
            return false;
        }
        grid_prog = *grid_prog_res;
    }

    {
        auto obj_prog_res = create_program_from_file("object");
        if (!obj_prog_res)
        {
            std::println(
                stderr,
                "Failed to load 'object' shaders, got error code: {}",
                static_cast<int>(obj_prog_res.error())
            );
            return false;
        }
        obj_prog = *obj_prog_res;
    }

    {
        auto outline_prog_res = create_program_from_file("outline");
        if (!outline_prog_res)
        {
            std::println(
                stderr,
                "Failed to load 'outline' shaders, got error code: {}",
                static_cast<int>(outline_prog_res.error())
            );
            return false;
        }
        outline_prog = *outline_prog_res;
    }

    {
        auto pivot_prog_res = create_program_from_file("pivot");
        if (!pivot_prog_res)
        {
            std::println(
                stderr,
                "Failed to load 'pivot' shaders, got error code: {}",
                std::to_underlying(pivot_prog_res.error())
            );
            return false;
        }
        pivot_prog = *pivot_prog_res;
    }

    if (!grid_prog.valid() || !obj_prog.valid() || !outline_prog.valid() || !pivot_prog.valid())
    {
        std::println(stderr, "At least one of the shader programs is invalid");
        return false;
    }

    return true;
}
}  // namespace ds_pba
