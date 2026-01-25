// pba/gfx/gfx_viewport.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/engine/scene_context.hpp"
#include "pba/engine/scene_types.hpp"
#include "pba/gfx/gl.hpp"
#include "pba/gfx/gl_types.hpp"
#include "pba/gfx/raycast.hpp"
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
void GfxContext::render_to_viewport_objects(
    const ds_pba::ViewMatrix& camera_view_matrix, const ds_pba::ProjMatrix& camera_proj_matrix
) const
{
    // Objects
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glStencilMask(0x00);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);

    obj_prog.bind();
    set_uniform_mat4(obj_prog.handle(), "uView", camera_view_matrix);
    set_uniform_mat4(obj_prog.handle(), "uProj", camera_proj_matrix);

    {  // Cubes
        cube_mesh.vao.bind();
        for (usize i{0zu}; i < scene_context->cube_objects.size(); ++i)
        {
            const Object& o{scene_context->cube_objects[i]};
            assert(o.id != k_invalid_id);

            set_uniform_mat4(obj_prog.handle(), "uModel", o.transform.model_matrix());
            set_uniform_vec3(obj_prog.handle(), "uColor", o.color);
            glDrawArrays(GL_TRIANGLES, 0, cube_mesh.vertex_count);
        }
        VAO::unbind();
    }

    constexpr const bool render_general_mesh{false};
    if constexpr (!render_general_mesh)
    {  // Spheres
        sphere_mesh.vao.bind();
        for (usize i{0zu}; i < scene_context->sphere_objects.size(); ++i)
        {
            const Object& o{scene_context->sphere_objects[i]};
            assert(o.id != k_invalid_id);

            set_uniform_mat4(obj_prog.handle(), "uModel", o.transform.model_matrix());
            set_uniform_vec3(obj_prog.handle(), "uColor", o.color);

            glDrawArrays(GL_TRIANGLES, 0, sphere_mesh.vertex_count);
        }
        for (usize i{0zu}; i < scene_context->hitmarker_objects.size(); ++i)
        {
            const Object& o{scene_context->hitmarker_objects[i]};
            assert(o.id != k_invalid_id);

            set_uniform_mat4(obj_prog.handle(), "uModel", o.transform.model_matrix());
            set_uniform_vec3(obj_prog.handle(), "uColor", o.color);

            glDrawArrays(GL_TRIANGLES, 0, sphere_mesh.vertex_count);
        }
        VAO::unbind();
    }
    if constexpr (render_general_mesh)
    {  // Currently no generic mesh support, this just spawns in first sphere pos and assumes
       // spheres are not rendered, so need to disable the previous scope
        assert(!scene_context->sphere_objects.empty());
        marble_bust_mesh.vao.bind();
        const Object& o{scene_context->sphere_objects[0]};
        set_uniform_mat4(obj_prog.handle(), "uModel", o.transform.model_matrix());
        set_uniform_vec3(obj_prog.handle(), "uColor", o.color);

        glDrawArrays(GL_TRIANGLES, 0, marble_bust_mesh.vertex_count);
        VAO::unbind();
    }
}
void GfxContext::render_to_viewport_grid(
    const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
) const
{
    // Grid
    glDepthMask(GL_FALSE);

    grid_prog.bind();
    set_uniform_mat4(grid_prog.handle(), "uView", camera_view_matrix);
    set_uniform_mat4(grid_prog.handle(), "uProj", camera_proj_matrix);
    set_uniform_float(grid_prog.handle(), "uFogStart", grid.fog_start);
    set_uniform_float(grid_prog.handle(), "uFogEnd", grid.fog_end);

    grid_mesh.vao.bind();
    glDrawArrays(GL_LINES, 0, grid_mesh.vertex_count);
    VAO::unbind();

    glDepthMask(GL_TRUE);
}

void GfxContext::render_to_viewport() const
{
    assert(viewport_fb_rect_valid && "Should only render to valid viewports");
    const ImVec2 content_size{ImGui::GetContentRegionAvail()};

    glBindFramebuffer(GL_FRAMEBUFFER, viewport_fbo.fbo);
    assert((viewport_fbo.width >= 0) && (viewport_fbo.height >= 0));
    glViewport(0, 0, viewport_fbo.width, viewport_fbo.height);

    ColorRGBAf bg{background_color};
    glClearColor(bg.r(), bg.g(), bg.b(), bg.a());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    f32 aspect{viewport_fbo.aspect_ratio()};

    const Camera& cam{scene_context->camera};

    const ViewMatrix camera_view_matrix{cam.view_matrix()};
    const ProjMatrix camera_proj_matrix{cam.proj_matrix(aspect)};

    render_to_viewport_grid(camera_view_matrix, camera_proj_matrix);
    render_to_viewport_objects(camera_view_matrix, camera_proj_matrix);
    render_to_viewport_pivot(cam.pivot, camera_view_matrix, camera_proj_matrix);
    render_to_viewport_outline(camera_view_matrix, camera_proj_matrix);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImGui::Image(
        viewport_fbo.imgui_texture_id(), content_size, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f)
    );
}

void GfxContext::render_to_viewport_outline(
    const ds_pba::ViewMatrix& camera_view_matrix, const ds_pba::ProjMatrix& camera_proj_matrix
) const
{
    if (!scene_context || scene_context->selected_ids.empty())
    {
        return;
    }

    auto find_object = [&](ObjectId id) -> std::optional<std::pair<ObjectType, const Object*>>
    {
        for (const Object& o : scene_context->cube_objects)
        {
            if (o.id == id)
            {
                return std::pair<ObjectType, const Object*>{ObjectType::Cube, &o};
            }
        }
        for (const Object& o : scene_context->sphere_objects)
        {
            if (o.id == id)
            {
                return std::pair<ObjectType, const Object*>{ObjectType::Sphere, &o};
            }
        }
        for (const Object& o : scene_context->hitmarker_objects)
        {
            if (o.id == id)
            {
                return std::pair<ObjectType, const Object*>{ObjectType::Hitmarker, &o};
            }
        }
        return std::nullopt;
    };

    auto instantiate_mesh_for_type = [&](ObjectType type) -> void
    {
        switch (type)
        {
            case ds_pba::ObjectType::Cube:
                cube_mesh.instantiate_once();
                break;
            case ds_pba::ObjectType::Sphere:
            case ds_pba::ObjectType::Hitmarker:
                sphere_mesh.instantiate_once();
                break;
        }
    };

    for (const ObjectId id : scene_context->selected_ids)
    {
        auto obj_res = find_object(id);
        if (!obj_res)
        {
            continue;
        }

        const auto [type, sel_ptr] = *obj_res;
        const Object& sel = *sel_ptr;

        const glm::mat4 M = sel.transform.model_matrix();

        glClear(GL_STENCIL_BUFFER_BIT);

        {  // Pass 1: write stencil
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_FALSE);
            glDisable(GL_DEPTH_TEST);

            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

            obj_prog.bind();
            set_uniform_mat4(obj_prog.handle(), "uView", camera_view_matrix);
            set_uniform_mat4(obj_prog.handle(), "uProj", camera_proj_matrix);
            set_uniform_mat4(obj_prog.handle(), "uModel", M);

            instantiate_mesh_for_type(type);

            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
        }

        {  // Pass 2: draw outline where stencil != 1
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);

            const ModelMatrix M_outline = M * glm::scale(glm::mat4(1.0f), glm::vec3(1.04f));

            outline_prog.bind();
            set_uniform_mat4(outline_prog.handle(), "uModel", M_outline);
            set_uniform_mat4(outline_prog.handle(), "uView", camera_view_matrix);
            set_uniform_mat4(outline_prog.handle(), "uProj", camera_proj_matrix);
            set_uniform_vec3(outline_prog.handle(), "uColor", glm::vec3(1.0f, 0.55f, 0.0f));

            instantiate_mesh_for_type(type);

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);

            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        }
    }
}

void GfxContext::render_to_viewport_pivot(
    const Position3& pivot_pos,
    const ViewMatrix& camera_view_matrix,
    const ProjMatrix& camera_proj_matrix
) const
{
    if (!pivot_active)
    {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    const GLMesh& pivot_mesh{sphere_mesh};

    pivot_prog.bind();
    set_uniform_mat4(pivot_prog.handle(), "uView", camera_view_matrix);
    set_uniform_mat4(pivot_prog.handle(), "uProj", camera_proj_matrix);

    pivot_mesh.vao.bind();

    const Transform t{.position = pivot_pos, .scale = {0.1f, 0.1f, 0.1f}};
    set_uniform_mat4(pivot_prog.handle(), "uModel", t.model_matrix());
    set_uniform_vec3(
        pivot_prog.handle(), "uColor", {196.0f / 255.0f, 209.0f / 255.0f, 102.0f / 255.0f}
    );

    glDrawArrays(GL_TRIANGLES, 0, pivot_mesh.vertex_count);

    VAO::unbind();
}

void GfxContext::viewport_window()
{
    ImGuiWindowFlags vp_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
                                | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Viewport", nullptr, vp_flags);
    const ImVec2 content_pos{ImGui::GetCursorScreenPos()};
    const ImVec2 content_size{ImGui::GetContentRegionAvail()};

    viewport_img_pos = glm::vec2{content_pos.x, content_pos.y};
    viewport_img_size = glm::vec2{content_size.x, content_size.y};

    viewport_fb_rect_valid = true;

    int fbw{0};
    int fbh{0};
    glfwGetFramebufferSize(window, &fbw, &fbh);
    if (fbw == 0 || fbh == 0)
    {
        std::println("[Warning] Got empty Framebuffer, skipping framebuffer setting");
        viewport_fb_rect_valid = false;
    }

    int win_w{1};
    int win_h{1};
    glfwGetWindowSize(window, &win_w, &win_h);
    if (win_w == 0 || win_h == 0)
    {
        std::println("[Warning] Got empty Window, skipping framebuffer setting");
        viewport_fb_rect_valid = false;
    }

    if (viewport_fb_rect_valid)
    {
        const auto win_w_f = static_cast<f32>(win_w);
        const auto win_h_f = static_cast<f32>(win_h);
        const auto fbw_f = static_cast<f32>(fbw);
        const auto fbh_f = static_cast<f32>(fbh);

        const f32 scale_x{(win_w_f > 0.0f) ? (fbw_f / win_w_f) : 1.0f};
        const f32 scale_y{(win_h_f > 0.0f) ? (fbh_f / win_h_f) : 1.0f};

        const f32 left_px{viewport_img_pos.x * scale_x};
        const f32 width_px{viewport_img_size.x * scale_x};
        const f32 height_px{viewport_img_size.y * scale_y};

        const f32 bottom_px{fbh_f - (viewport_img_pos.y + viewport_img_size.y) * scale_y};

        const auto vx = static_cast<int>(std::lround(left_px));
        const auto vw = static_cast<int>(std::lround(width_px));
        const auto vy = static_cast<int>(std::lround(bottom_px));
        const auto vh = static_cast<int>(std::lround(height_px));

        viewport_fb_rect.x = std::clamp(vx, 0, std::max(0, fbw - 1));
        viewport_fb_rect.y = std::clamp(vy, 0, std::max(0, fbh - 1));
        viewport_fb_rect.width = std::clamp(vw, 1, fbw - viewport_fb_rect.x);
        viewport_fb_rect.height = std::clamp(vh, 1, fbh - viewport_fb_rect.y);

        const int fbo_w{viewport_fb_rect.width};
        const int fbo_h{viewport_fb_rect.height};
        viewport_fb_rect_valid = (fbo_w > 8 && fbo_h > 8) && viewport_fbo.ensure_size(fbo_w, fbo_h);
    }

    if (viewport_fb_rect_valid)
    {
        viewport_valid_warning_shown = false;
        render_to_viewport();
        viewport_image_hovered =
            ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        viewport_image_hovered = false;
        if (!viewport_valid_warning_shown)
        {
            std::println("[Warning] Viewport is not valid!");
            viewport_valid_warning_shown = true;
        }
    }
    ImGui::End();
}
}  // namespace ds_pba
