// pba/gfx/gfx_gl_resources.cpp
#include "pba/core/constants.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/assets/gltf_mesh.hpp"
#include "pba/assets/mesh.hpp"
#include "pba/assets/mesh_data.hpp"
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/gfx/gl.hpp"
#include "pba/gfx/gl_types.hpp"
#include "pba/ui/ui.hpp"
#include "pba/util/scope_timer.hpp"
//
#include <imgui.h>
#include <optional>
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/geometric.hpp>
#include <json.hpp>

namespace ds_pba
{
namespace
{
[[nodiscard]] auto lerp_color(const Color3& a, const Color3& b, f32 t) noexcept -> Color3
{
    return Color3{
        a.r() + (b.r() - a.r()) * t,
        a.g() + (b.g() - a.g()) * t,
        a.b() + (b.b() - a.b()) * t,
    };
}

[[nodiscard]] auto to_u8_channel(f32 linear_value) noexcept -> u8
{
    const auto clamped = std::clamp(linear_value, 0.0f, 1.0f);
    const auto scaled = static_cast<int>(std::lround(clamped * 255.0f));
    return static_cast<u8>(std::clamp(scaled, 0, 255));
}

[[nodiscard]] auto make_default_environment_map_rgba8(int width, int height) -> ImageRGBA8
{
    {
        Expects(width > 0);
        Expects(height > 0);
    }
    const auto width_f = static_cast<f32>(width);
    const auto height_f = static_cast<f32>(height);

    ImageRGBA8 img{};
    img.width = width;
    img.height = height;
    img.channels = 4;

    const auto pixel_count = width_f * height_f;
    img.pixels.resize(pixel_count * 4zu);

    constexpr Color3 sky_zenith{0.08f, 0.22f, 0.45f};
    constexpr Color3 sky_horizon{0.62f, 0.74f, 0.86f};
    constexpr Color3 ground{0.18f, 0.17f, 0.16f};
    const auto sun_dir = glm::normalize(Dir3{0.25f, -0.35f, 0.90f});

    for (int y{0}; y < height; ++y)
    {
        const f32 y_f = static_cast<f32>(y);
        const auto v = (y_f + 0.5f) / height_f;
        const auto theta = v * k_pi;
        const auto sin_theta = std::sin(theta);
        const auto cos_theta = std::cos(theta);

        for (int x{0}; x < width; ++x)
        {
            const auto u = (x_f + 0.5f) / width_f;
            const auto phi = (u - 0.5f) * k_two_pi;

            const auto dir = Dir3{
                std::cos(phi) * sin_theta,
                std::sin(phi) * sin_theta,
                cos_theta,
            };

            const auto sky_t = std::clamp(0.5f * (dir.z + 1.0f), 0.0f, 1.0f);
            const auto sky_curve = std::pow(sky_t, 0.65f);
            const auto sky = lerp_color(sky_horizon, sky_zenith, sky_curve);

            const auto horizon_blend = std::clamp((dir.z + 0.08f) / 0.18f, 0.0f, 1.0f);
            auto base = lerp_color(ground, sky, horizon_blend);

            const auto sun_cos = std::max(glm::dot(dir, sun_dir), 0.0f);
            const auto sun_glow = std::pow(sun_cos, 48.0f);
            const auto sun_core = std::pow(sun_cos, 1024.0f);

            base.r() += 0.40f * sun_glow + 1.20f * sun_core;
            base.g() += 0.33f * sun_glow + 0.95f * sun_core;
            base.b() += 0.22f * sun_glow + 0.55f * sun_core;

            const auto idx =
                (static_cast<usize>(y) * static_cast<usize>(width) + static_cast<usize>(x)) * 4zu;
            img.pixels[idx + 0zu] = to_u8_channel(base.r());
            img.pixels[idx + 1zu] = to_u8_channel(base.g());
            img.pixels[idx + 2zu] = to_u8_channel(base.b());
            img.pixels[idx + 3zu] = 255u;
        }
    }

    {
        Ensures(img.valid());
    }
    return img;
}

}  // namespace

auto upload_mesh_pcolor_lines(const MeshDataPColor& mesh_data) -> std::optional<GLMesh>
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

auto upload_mesh_pnt(const MeshDataPNT& mesh_data) -> std::optional<GLMesh>
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

auto upload_mesh_pn(const MeshDataPN& mesh_data) -> std::optional<GLMesh>
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

auto GfxContext::create_textures() -> bool
{
    ScopeTimer timer{"create_textures"};
    namespace fs = std::filesystem;

    {  // Environment Lighting
        const fs::path env_path = fs::path{k_fp_assets} / k_fp_assets_textures / k_texture_environment
                                  / k_texture_environment_latlong;
        std::error_code ec{};
        const bool has_env_asset = fs::is_regular_file(env_path, ec) && !ec;

        auto env_img = has_env_asset
                           ? load_image_rgba8(
                                 env_path, TextureLoadOptions{.flip_y = false, .force_rgba = true}
                             )
                           : std::optional<ImageRGBA8>{
                                 make_default_environment_map_rgba8(1024, 512)
                             };
        if (!env_img)
        {
            std::println(stderr, "Failed to prepare environment texture '{}'", env_path.string());
            return false;
        }

        auto tex = upload_texture_2d_rgba8(*env_img, {.generate_mips = true, .srgb = false});
        if (!tex)
        {
            std::println(stderr, "Failed to upload environment texture '{}'", env_path.string());
            return false;
        }

        textures.environment_lighting = *tex;
    }

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

auto GfxContext::create_meshes() -> bool
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

auto GfxContext::create_programs() -> bool
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
    if (!load_prog("grid",        shader_programs.grid))        return false;
    if (!load_prog("environment", shader_programs.environment)) return false;
    if (!load_prog("object",      shader_programs.obj))         return false;
    if (!load_prog("object_tex",  shader_programs.obj_tex))     return false;
    if (!load_prog("outline",     shader_programs.outline))     return false;
    if (!load_prog("pivot",       shader_programs.pivot))       return false;
    // clang-format on

    if (!shader_programs.all_valid())
    {
        std::println(stderr, "At least one of the shader programs is invalid");
        return false;
    }
    return true;
}
}  // namespace ds_pba
