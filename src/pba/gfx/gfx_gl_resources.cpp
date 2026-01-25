// pba/gfx/gfx_gl_resources.cpp
#include "pba/core/constants.hpp"
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
        meshes.cube = *mesh_res;
    }
    {
        auto mesh_res = upload_or_fail_pn(create_sphere_mesh(32, 24, 1.0f), "sphere");
        if (!mesh_res)
        {
            return false;
        }
        meshes.sphere = *mesh_res;
    }
    {
        auto mesh_res = upload_or_fail_grid(create_grid_mesh(grid), "grid");
        if (!mesh_res)
        {
            return false;
        }
        meshes.grid = *mesh_res;
    }
    {
        auto mesh_res = upload_or_fail_pn(create_cylinder_mesh(24, 0.5f, 1.0f), "cylinder");
        if (!mesh_res)
        {
            return false;
        }
        meshes.cylinder = *mesh_res;
    }
    {
        auto mesh_res = upload_or_fail_pn_value(create_pyramid_mesh(), "pyramid");
        if (!mesh_res)
        {
            return false;
        }
        meshes.pyramid = *mesh_res;
    }
    {
        auto mesh_res = ds_pba::load_model_mesh(k_model_marble_bust);
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
        meshes.marble_bust = *m;
    }

    return true;
}

bool GfxContext::create_programs()
{

    {
        auto grid_res = create_program_from_file("grid");
        if (!grid_res)
        {
            std::println(
                stderr,
                "Failed to load 'grid' shaders, got error code: {}",
                std::to_underlying(grid_res.error())
            );
            return false;
        }
        shader_programs.grid = *grid_res;
    }

    {
        auto obj_res = create_program_from_file("object");
        if (!obj_res)
        {
            std::println(
                stderr,
                "Failed to load 'object' shaders, got error code: {}",
                static_cast<int>(obj_res.error())
            );
            return false;
        }
        shader_programs.obj = *obj_res;
    }

    {
        auto outline_res = create_program_from_file("outline");
        if (!outline_res)
        {
            std::println(
                stderr,
                "Failed to load 'outline' shaders, got error code: {}",
                static_cast<int>(outline_res.error())
            );
            return false;
        }
        shader_programs.outline = *outline_res;
    }

    {
        auto pivot_res = create_program_from_file("pivot");
        if (!pivot_res)
        {
            std::println(
                stderr,
                "Failed to load 'pivot' shaders, got error code: {}",
                std::to_underlying(pivot_res.error())
            );
            return false;
        }
        shader_programs.pivot = *pivot_res;
    }

    if (!shader_programs.grid.valid() || !shader_programs.obj.valid() || !shader_programs.outline.valid() || !shader_programs.pivot.valid())
    {
        std::println(stderr, "At least one of the shader programs is invalid");
        return false;
    }

    return true;
}
}  // namespace ds_pba
