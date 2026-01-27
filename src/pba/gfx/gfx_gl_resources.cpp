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
#include "pba/util/scope_timer.hpp"
//
#include <imgui.h>
#include <optional>
#include <print>
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <json.hpp>

namespace ds_pba
{
[[nodiscard]] std::optional<GLMesh> upload_mesh_pcolor_lines(const MeshDataPColor& mesh_data)
{
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

[[nodiscard]] std::optional<GLMesh> upload_mesh_pnt(const MeshDataPNT& mesh_data)
{
    const auto& verts = mesh_data.vertices;
    if (verts.empty())
    {
        std::println(stderr, "Mesh (PNT) data empty!");
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
            static_cast<GLsizeiptr>(verts.size() * sizeof(MeshV_PNT)),
            verts.data(),
            GL_STATIC_DRAW
        );

        const auto stride = static_cast<GLsizei>(sizeof(MeshV_PNT));

        // location 0: position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, GLPtr::offset0());

        // location 1: normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, GLPtr::offset(3 * sizeof(f32)));

        // location 2: texcoord
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, GLPtr::offset(6 * sizeof(f32)));
    }

    return mesh;
}

[[nodiscard]] std::optional<GLMesh> upload_mesh_pn(const MeshDataPN& mesh_data)
{
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

bool GfxContext::create_textures()
{
    ScopeTimer timer{"create_textures"};
    namespace fs = std::filesystem;

    if constexpr (false)
    {  // Asphalt
        const fs::path texture_dir =
            fs::path{k_fp_assets} / k_fp_assets_textures / k_texture_clean_asphalt;

        const fs::path diffuse_path = texture_dir / k_texture_clean_asphalt_diffuse;
        const fs::path normal_path = texture_dir / k_texture_clean_asphalt_normal;

        {
            auto img = load_image_rgba8(diffuse_path);
            if (!img)
            {
                std::println(stderr, "Failed to load '{}'", diffuse_path.string());
                return false;
            }

            auto tex = upload_texture_2d_rgba8(*img, {.generate_mips = true, .srgb = true});
            if (!tex)
            {
                std::println(stderr, "Failed to upload '{}'", diffuse_path.string());
                return false;
            }

            textures.clean_asphalt_diffuse = *tex;
        }

        {
            auto img = load_image_rgba8(normal_path);
            if (!img)
            {
                std::println(stderr, "Failed to load '{}'", normal_path.string());
                return false;
            }

            auto tex = upload_texture_2d_rgba8(*img, {.generate_mips = true, .srgb = false});
            if (!tex)
            {
                std::println(stderr, "Failed to upload '{}'", normal_path.string());
                return false;
            }

            textures.clean_asphalt_normal = *tex;
        }
    }

    if constexpr (false)
    {  // Marble Bust
        const fs::path texture_dir =
            fs::path{k_fp_assets} / k_fp_assets_textures / k_texture_marble_bust_2k;

        const fs::path diffuse_path = texture_dir / k_texture_marble_bust_diffuse;
        const fs::path normal_path = texture_dir / k_texture_marble_bust_normal;

        {
            auto img = load_image_rgba8(
                diffuse_path, TextureLoadOptions{.flip_y = false, .force_rgba = true}
            );
            if (!img)
            {
                std::println(stderr, "Failed to load '{}'", diffuse_path.string());
                return false;
            }

            auto tex = upload_texture_2d_rgba8(*img, {.generate_mips = true, .srgb = true});
            if (!tex)
            {
                std::println(stderr, "Failed to upload '{}'", diffuse_path.string());
                return false;
            }

            textures.marble_bust_diffuse = *tex;
        }

        {
            auto img = load_image_rgba8(
                normal_path, TextureLoadOptions{.flip_y = false, .force_rgba = true}
            );
            if (!img)
            {
                std::println(stderr, "Failed to load '{}'", normal_path.string());
                return false;
            }

            auto tex = upload_texture_2d_rgba8(*img, {.generate_mips = true, .srgb = false});
            if (!tex)
            {
                std::println(stderr, "Failed to upload '{}'", normal_path.string());
                return false;
            }

            textures.marble_bust_normal = *tex;
        }
    }
    return true;
}

bool GfxContext::create_meshes()
{
    ScopeTimer t{"create_meshes"};
    const auto upload_pn_or_fail =
        [&](const MeshDataPN& mesh_data, std::string_view label, GLMesh& dst) -> bool
    {
        auto mesh_res = upload_mesh_pn(mesh_data);
        if (!mesh_res)
        {
            std::println(stderr, "Failed to upload mesh '{}'", label);
            return false;
        }
        dst = *mesh_res;
        return true;
    };
    const auto upload_pnt_or_fail =
        [&](const MeshDataPNT& mesh_data, std::string_view label, GLMesh& dst) -> bool
    {
        auto mesh_res = upload_mesh_pnt(mesh_data);
        if (!mesh_res)
        {
            std::println(stderr, "Failed to upload mesh '{}'", label);
            return false;
        }
        dst = *mesh_res;
        return true;
    };
    const auto upload_grid_or_fail =
        [&](const MeshDataPColor& mesh_data, std::string_view label, GLMesh& dst) -> bool
    {
        auto mesh_res = upload_mesh_pcolor_lines(mesh_data);
        if (!mesh_res)
        {
            std::println(stderr, "Failed to upload mesh '{}'", label);
            return false;
        }
        dst = *mesh_res;
        return true;
    };
    {  // Cube
        const MeshDataPN mesh = create_cube_mesh();
        if (!upload_pn_or_fail(mesh, "cube", meshes.cube))
        {
            return false;
        }
    }
    {  // Sphere
        const MeshDataPN mesh = create_sphere_mesh(32, 24, 1.0f);
        if (!upload_pn_or_fail(mesh, "sphere", meshes.sphere))
        {
            return false;
        }
    }
    {  // Grid
        const MeshDataPColor mesh = create_grid_mesh(grid);
        if (!upload_grid_or_fail(mesh, "grid", meshes.grid))
        {
            return false;
        }
    }
    {  // Cylinder
        const MeshDataPN mesh = create_cylinder_mesh(24, 0.5f, 1.0f);
        if (!upload_pn_or_fail(mesh, "cylinder", meshes.cylinder))
        {
            return false;
        }
    }
    {  // Pyramid
        const MeshDataPN mesh = create_pyramid_mesh();
        if (!upload_pn_or_fail(mesh, "pyramid", meshes.pyramid))
        {
            return false;
        }
    }
    if constexpr (false)
    {  // Marble Bust
        auto mesh_res = load_model_mesh_pnt(k_model_marble_bust);
        if (!mesh_res)
        {
            std::println(stderr, "Failed to load '{}' mesh (PNT)", k_model_marble_bust);
            return false;
        }

        if (!upload_pnt_or_fail(*mesh_res, k_model_marble_bust, meshes.marble_bust))
        {
            return false;
        }
    }
    return true;
}

bool GfxContext::create_programs()
{
    ScopeTimer timer{"create_programs"};
    const auto load_prog = [&](std::string_view name, ShaderProgram& out) -> bool
    {
        auto res = create_program_from_file(std::string{name});
        if (!res)
        {
            std::println(stderr, "Failed to load '{}' shaders", name);
            return false;
        }
        out = *res;
        return true;
    };

    // clang-format off
    if (!load_prog("grid",       shader_programs.grid))    return false;
    if (!load_prog("object",     shader_programs.obj))     return false;
    if (!load_prog("object_tex", shader_programs.obj_tex)) return false;
    if (!load_prog("outline",    shader_programs.outline)) return false;
    if (!load_prog("pivot",      shader_programs.pivot))   return false;
    // clang-format on

    if (!shader_programs.all_valid())
    {
        std::println(stderr, "At least one of the shader programs is invalid");
        return false;
    }
    return true;
}
}  // namespace ds_pba
