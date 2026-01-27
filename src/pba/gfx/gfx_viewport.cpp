// pba/gfx/gfx_viewport.cpp
#include "pba/core/geometry.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/engine/engine_context.hpp"
#include "pba/engine/scene_context.hpp"
#include "pba/engine/scene_types.hpp"
#include "pba/gfx/gl.hpp"
#include "pba/gfx/gl_types.hpp"
#include "pba/gfx/raycast.hpp"
#include "pba/ui/ui.hpp"
//
#include <algorithm>
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
    const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
) const
{
    // Objects
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glStencilMask(0x00);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);

    shader_programs.obj.bind();
    set_uniform_mat4(shader_programs.obj.handle(), "uView", camera_view_matrix.m);
    set_uniform_mat4(shader_programs.obj.handle(), "uProj", camera_proj_matrix.m);

    if (!scene_context->cube_objects.empty())
    {  // Cubes
        meshes.cube.vao.bind();
        for (usize i{0zu}; i < scene_context->cube_objects.size(); ++i)
        {
            const Object& o{scene_context->cube_objects[i]};
            assert(o.id != k_invalid_id);

            set_uniform_mat4(shader_programs.obj.handle(), "uModel", o.transform.model_matrix().m);

            // TODO: Clean this up
            // TODO: Clean this up
            // TODO: Clean this up
            // TODO: Clean this up
            // TODO: Clean this up
            // TODO: Clean this up
            // TODO: Clean this up
            // TODO: Clean this up
            // TODO: Clean this up
            // TODO: Clean this up
            // TODO: Clean this up
            // TODO: Clean this up
            // TODO: Clean this up
            {  // Color
                Color3 color{o.color};
                if (phys_debug.enabled && engine_context)
                {
                    if (auto it = engine_context->obj_map.find(o.id);
                        it != engine_context->obj_map.end())
                    {
                        const usize phys_i{it->second.physics_obj_idx};
                        if (phys_i < engine_context->physics.bodies.size())
                        {
                            const RigidBody& rb = engine_context->physics.bodies[phys_i];
                            if (!rb.is_static())
                            {
                                switch (phys_debug.color_mode)
                                {
                                    case PhysicsDebugSettings::ColorMode::Diffuse:
                                        break;

                                    case PhysicsDebugSettings::ColorMode::SleepState:
                                        {
                                            const f32 t =
                                                rb.asleep
                                                    ? 1.0f
                                                    : std::clamp(
                                                          static_cast<f32>(rb.sleep_frames) / 60.0f,
                                                          0.0f,
                                                          1.0f
                                                      );
                                            color =
                                                mix(phys_debug.sleep_active_color,
                                                    phys_debug.sleep_asleep_color,
                                                    t);
                                        }
                                        break;

                                    case PhysicsDebugSettings::ColorMode::KineticEnergy:
                                        {
                                            const auto m = 1.0f / rb.inv_mass;
                                            auto E = 0.5f * m * glm::dot(rb.velocity, rb.velocity);

                                            if (phys_debug.ke_include_angular)
                                            {
                                                const glm::mat3 I_world =
                                                    glm::inverse(rb.inv_inertia_world);
                                                E += 0.5f
                                                     * glm::dot(
                                                         rb.angular_velocity,
                                                         I_world * rb.angular_velocity
                                                     );
                                            }

                                            const auto denom = std::max(1e-6f, phys_debug.ke_max);
                                            const auto t = std::clamp(E / denom, 0.0f, 1.0f);
                                            color = mix(
                                                phys_debug.ke_low_color, phys_debug.ke_high_color, t
                                            );
                                        }
                                        break;
                                }
                            }
                        }
                    }
                }
                set_uniform_color3(shader_programs.obj.handle(), "uColor", color);
            }

            glDrawArrays(GL_TRIANGLES, 0, meshes.cube.vertex_count);
        }
        VAO::unbind();
    }

    if (!scene_context->sphere_objects.empty())
    {  // Spheres
        meshes.sphere.vao.bind();
        for (usize i{0zu}; i < scene_context->sphere_objects.size(); ++i)
        {
            const Object& o{scene_context->sphere_objects[i]};
            assert(o.id != k_invalid_id);

            set_uniform_mat4(shader_programs.obj.handle(), "uModel", o.transform.model_matrix().m);
            set_uniform_color3(shader_programs.obj.handle(), "uColor", o.color);

            glDrawArrays(GL_TRIANGLES, 0, meshes.sphere.vertex_count);
        }
        for (const auto& o : scene_context->hitmarker_objects)
        {
            assert(o.id != k_invalid_id);
            set_uniform_mat4(shader_programs.obj.handle(), "uModel", o.transform.model_matrix().m);
            set_uniform_color3(shader_programs.obj.handle(), "uColor", o.color);

            glDrawArrays(GL_TRIANGLES, 0, meshes.sphere.vertex_count);
        }
        VAO::unbind();
    }

    // Temp deleted so I can avoid loading textures and meshes on start up improving
    // startup speed significantly
    if (false && !scene_context->marble_bust_objects.empty())
    {  // Marble Bust
        if (!textures.marble_bust_diffuse.valid() || !textures.marble_bust_normal.valid())
        {
            std::println(
                stderr, "Textured mesh render requires diffuse+normal textures to be loaded"
            );
            VAO::unbind();
            return;
        }

        shader_programs.obj_tex.bind();
        set_uniform_mat4(shader_programs.obj_tex.handle(), "uView", camera_view_matrix.m);
        set_uniform_mat4(shader_programs.obj_tex.handle(), "uProj", camera_proj_matrix.m);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures.marble_bust_diffuse.id);
        set_uniform_int(shader_programs.obj_tex.handle(), "uDiffuseTex", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures.marble_bust_normal.id);
        set_uniform_int(shader_programs.obj_tex.handle(), "uNormalTex", 1);

        meshes.marble_bust.vao.bind();

        for (const auto& o : scene_context->marble_bust_objects)
        {
            assert(o.id != k_invalid_id);
            set_uniform_mat4(
                shader_programs.obj_tex.handle(), "uModel", o.transform.model_matrix().m
            );
            glDrawArrays(GL_TRIANGLES, 0, meshes.marble_bust.vertex_count);
        }

        VAO::unbind();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}
void GfxContext::render_to_viewport_grid(
    const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
) const
{
    // Grid
    glDepthMask(GL_FALSE);

    shader_programs.grid.bind();
    set_uniform_mat4(shader_programs.grid.handle(), "uView", camera_view_matrix.m);
    set_uniform_mat4(shader_programs.grid.handle(), "uProj", camera_proj_matrix.m);
    set_uniform_float(shader_programs.grid.handle(), "uFogStart", grid.fog_start);
    set_uniform_float(shader_programs.grid.handle(), "uFogEnd", grid.fog_end);

    meshes.grid.vao.bind();
    glDrawArrays(GL_LINES, 0, meshes.grid.vertex_count);
    VAO::unbind();

    glDepthMask(GL_TRUE);
}

void GfxContext::render_to_viewport()
{
    Expects(viewport_fb_rect_valid && "Should only render to valid viewports");
    const ImVec2 content_size{ImGui::GetContentRegionAvail()};

    glBindFramebuffer(GL_FRAMEBUFFER, viewport_fbo.fbo);
    assert((viewport_fbo.width >= 0) && (viewport_fbo.height >= 0));
    glViewport(0, 0, viewport_fbo.width, viewport_fbo.height);

    ColorRGBAf bg{background_color};
    glClearColor(bg.r(), bg.g(), bg.b(), bg.a());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    f32 aspect{viewport_fbo.aspect_ratio()};

    {
        const Camera& cam{scene_context->camera};
        const ViewMatrix camera_view_matrix{cam.view_matrix()};
        const ProjMatrix camera_proj_matrix{cam.proj_matrix(aspect)};

        render_to_viewport_grid(camera_view_matrix, camera_proj_matrix);
        render_to_viewport_objects(camera_view_matrix, camera_proj_matrix);
        render_to_viewport_pivot(cam.pivot, camera_view_matrix, camera_proj_matrix);
        render_to_viewport_outline(camera_view_matrix, camera_proj_matrix);
        render_to_viewport_physics_debug(camera_view_matrix, camera_proj_matrix);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImGui::Image(
        viewport_fbo.imgui_texture_id(), content_size, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f)
    );
}

void GfxContext::render_to_viewport_outline(
    const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
) const
{
    if (!scene_context || scene_context->selected_ids.empty())
    {
        return;
    }

    struct FoundObject
    {
        ObjectType type{};
        const Object* object{};
    };
    struct ObjectBucket
    {
        ObjectType type;
        const std::vector<Object>* objects;
    };

    const std::array<ObjectBucket, 4> buckets{{
        {ObjectType::Cube, &scene_context->cube_objects},
        {ObjectType::Sphere, &scene_context->sphere_objects},
        {ObjectType::Hitmarker, &scene_context->hitmarker_objects},
        {ObjectType::MarbleBust, &scene_context->marble_bust_objects},
    }};

    auto find_object = [&](ObjectId id) -> std::optional<FoundObject>
    {
        for (const auto& b : buckets)
        {
            for (const Object& o : *b.objects)
            {
                if (o.id == id)
                {
                    return FoundObject{b.type, &o};
                }
            }
        }
        return std::nullopt;
    };

    auto instantiate_mesh_for_type = [&](ObjectType type) -> void
    {
        switch (type)
        {
            case ObjectType::Cube:
                meshes.cube.instantiate_once();
                break;
            case ObjectType::Sphere:
            case ObjectType::Hitmarker:
                meshes.sphere.instantiate_once();
                break;
            case ObjectType::MarbleBust:
                meshes.marble_bust.instantiate_once();
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

        const auto model_matrix{sel.transform.model_matrix()};

        glClear(GL_STENCIL_BUFFER_BIT);

        {  // Pass 1: write stencil
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_FALSE);
            glDisable(GL_DEPTH_TEST);

            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

            shader_programs.obj.bind();
            set_uniform_mat4(shader_programs.obj.handle(), "uView", camera_view_matrix.m);
            set_uniform_mat4(shader_programs.obj.handle(), "uProj", camera_proj_matrix.m);
            set_uniform_mat4(shader_programs.obj.handle(), "uModel", model_matrix.m);

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

            const auto M_outline =
                ModelMatrix{model_matrix.m * glm::scale(glm::mat4(1.0f), glm::vec3(1.04f))};

            shader_programs.outline.bind();
            set_uniform_mat4(shader_programs.outline.handle(), "uModel", M_outline.m);
            set_uniform_mat4(shader_programs.outline.handle(), "uView", camera_view_matrix.m);
            set_uniform_mat4(shader_programs.outline.handle(), "uProj", camera_proj_matrix.m);
            set_uniform_vec3(
                shader_programs.outline.handle(), "uColor", glm::vec3(1.0f, 0.55f, 0.0f)
            );

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
    const Pos3& pivot_pos,
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

    const GLMesh& pivot_mesh{meshes.sphere};

    shader_programs.pivot.bind();
    set_uniform_mat4(shader_programs.pivot.handle(), "uView", camera_view_matrix.m);
    set_uniform_mat4(shader_programs.pivot.handle(), "uProj", camera_proj_matrix.m);

    pivot_mesh.vao.bind();

    const Transform t{.position = pivot_pos, .scale = {0.1f, 0.1f, 0.1f}};
    set_uniform_mat4(shader_programs.pivot.handle(), "uModel", t.model_matrix().m);
    set_uniform_vec3(
        shader_programs.pivot.handle(),
        "uColor",
        {196.0f / 255.0f, 209.0f / 255.0f, 102.0f / 255.0f}
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

    int fbw{0}, fbh{0};
    glfwGetFramebufferSize(window, &fbw, &fbh);
    if (fbw == 0 || fbh == 0)
    {
        std::println("[Warning] Got empty Framebuffer, skipping framebuffer setting");
        viewport_fb_rect_valid = false;
    }

    int win_w{1}, win_h{1};
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

void GfxContext::render_to_viewport_physics_debug(
    const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
)
{
    if (!phys_debug.enabled)
    {
        return;
    }
    if (!scene_context || !engine_context)
    {
        return;
    }
    if (!shader_programs.grid.valid() || !loaded_glad)
    {
        return;
    }

    debug_line_vertices.clear();

    auto push_line = [&](const Pos3& a, const Pos3& b, f32 r, f32 g, f32 bl, f32 al) -> void
    {
        debug_line_vertices.push_back(DebugLineV_PColor{a.x, a.y, a.z, r, g, bl, al});
        debug_line_vertices.push_back(DebugLineV_PColor{b.x, b.y, b.z, r, g, bl, al});
    };

    if (phys_debug.show_contacts)
    {
        const PhysicsContext& phys = engine_context->physics;
        const f32 s{std::max(0.0f, phys_debug.contact_marker_size)};
        const f32 nscale{std::max(0.0f, phys_debug.contact_normal_scale)};

        for (const auto& c : phys.debug_contacts)
        {
            const Pos3 p{c.p};

            push_line(p + Dir3{s, 0.0f, 0.0f}, p - Dir3{s, 0.0f, 0.0f}, 1, 1, 1, 1);
            push_line(p + Dir3{0.0f, s, 0.0f}, p - Dir3{0.0f, s, 0.0f}, 1, 1, 1, 1);
            push_line(p + Dir3{0.0f, 0.0f, s}, p - Dir3{0.0f, 0.0f, s}, 1, 1, 1, 1);

            if (phys_debug.show_contact_normals)
            {
                push_line(p, p + c.n * nscale, 1.0f, 0.75f, 0.10f, 1.0f);
            }
        }
    }

    auto find_scene_object = [&](ObjectId id) -> const Object*
    {
        for (const Object& o : scene_context->cube_objects)
        {
            if (o.id == id)
                return &o;
        }
        for (const Object& o : scene_context->sphere_objects)
        {
            if (o.id == id)
                return &o;
        }
        for (const Object& o : scene_context->hitmarker_objects)
        {
            if (o.id == id)
                return &o;
        }
        return nullptr;
    };

    for (const ObjectId id : scene_context->selected_ids)
    {
        const RigidBody* rb_ptr{nullptr};

        if (auto it = engine_context->obj_map.find(id); it != engine_context->obj_map.end())
        {
            const usize phys_i = it->second.physics_obj_idx;
            if (phys_i < engine_context->physics.bodies.size())
            {
                rb_ptr = &engine_context->physics.bodies[phys_i];
            }
        }

        Pos3 com{};
        Quaternion ori{};
        f32 extent{1.0f};

        if (rb_ptr)
        {
            const RigidBody& rb{*rb_ptr};
            com = rb.position;
            ori = rb.orientation;

            extent = 2.0f * std::max({rb.half_extents.x, rb.half_extents.y, rb.half_extents.z});
        }
        else
        {
            const Object* o = find_scene_object(id);
            if (!o)
            {
                continue;
            }
            com = o->transform.position;
            ori = o->transform.orientation;

            extent = std::max({o->transform.scale.x, o->transform.scale.y, o->transform.scale.z});
        }

        const f32 axis_len = std::max(0.0f, extent) * std::max(0.0f, phys_debug.axis_scale);

        const glm::mat3 R{glm::mat3_cast(ori)};
        const Dir3 ax{glm::normalize(R * k_axis_x)};
        const Dir3 ay{glm::normalize(R * k_axis_y)};
        const Dir3 az{glm::normalize(R * k_axis_z)};

        if (phys_debug.show_selected_axes)
        {
            push_line(com, com + axis_len * ax, 1, 0, 0, 1);
            push_line(com, com + axis_len * ay, 0, 1, 0, 1);
            push_line(com, com + axis_len * az, 0, 0, 1, 1);
        }

        if (rb_ptr)
        {
            const RigidBody& rb{*rb_ptr};

            if (phys_debug.show_selected_velocity)
            {
                const f32 v2{glm::dot(rb.velocity, rb.velocity)};
                if (v2 > 1e-8f)
                {
                    push_line(com, com + rb.velocity * phys_debug.vel_scale, 1, 1, 0, 1);
                }
            }

            if (phys_debug.show_selected_angular_velocity)
            {
                const f32 w2{glm::dot(rb.angular_velocity, rb.angular_velocity)};
                if (w2 > 1e-8f)
                {
                    push_line(
                        com, com + rb.angular_velocity * phys_debug.ang_vel_scale, 0, 1, 1, 1
                    );
                }
            }
        }
    }

    if (debug_line_vertices.empty())
    {
        return;
    }

    if (!debug_line_created)
    {
        GLMesh m{};
        glGenVertexArrays(1, m.vao.ptr());
        glGenBuffers(1, m.vbo.ptr());
        m.vertex_count = 0;

        {
            const ScopedBufferBinder binder{m};

            const GLsizei stride = static_cast<GLsizei>(sizeof(DebugLineV_PColor));
            glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, GLPtr::offset0());

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, GLPtr::offset(3 * sizeof(f32)));
        }

        meshes.debug_line = m;
        debug_line_created = true;
    }

    meshes.debug_line.vertex_count = static_cast<GLsizei>(debug_line_vertices.size());

    {
        const ScopedBufferBinder binder{meshes.debug_line};
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(debug_line_vertices.size() * sizeof(DebugLineV_PColor)),
            debug_line_vertices.data(),
            GL_DYNAMIC_DRAW
        );
    }

    const GLboolean prev_depth_test = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean prev_cull_face = glIsEnabled(GL_CULL_FACE);
    const GLboolean prev_stencil_test = glIsEnabled(GL_STENCIL_TEST);

    if (phys_debug.depth_test)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);

    glDisable(GL_STENCIL_TEST);

    shader_programs.grid.bind();
    set_uniform_mat4(shader_programs.grid.handle(), "uView", camera_view_matrix.m);
    set_uniform_mat4(shader_programs.grid.handle(), "uProj", camera_proj_matrix.m);

    set_uniform_float(shader_programs.grid.handle(), "uFogStart", 1.0e6f);
    set_uniform_float(shader_programs.grid.handle(), "uFogEnd", 2.0e6f);

    meshes.debug_line.vao.bind();
    glDrawArrays(GL_LINES, 0, meshes.debug_line.vertex_count);
    VAO::unbind();

    if (prev_depth_test)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    if (prev_cull_face)
    {
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }

    if (prev_stencil_test)
    {
        glEnable(GL_STENCIL_TEST);
    }
    else
    {
        glDisable(GL_STENCIL_TEST);
    }
}

}  // namespace ds_pba
