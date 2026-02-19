// pba/gfx/gfx_viewport.cpp
#include "glm/geometric.hpp"
#include "pba/core/geometry.hpp"
#include "pba/core/math_types.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/engine/engine_context.hpp"
#include "pba/gfx/gl_types.hpp"
#include "pba/simulation/simulation_context.hpp"
//
#include <algorithm>
#include <cmath>
#include <optional>
#include <print>
//
#include <gsl/assert>
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_transform.hpp>
#include <imgui.h>

namespace ds_pba
{
namespace
{
[[nodiscard]] auto compute_viewport_fb_rect(
    GLFWwindow& window, const glm::vec2& img_pos_screen, const glm::vec2& img_size_screen
) noexcept -> std::optional<RectInt>
{
    int fb_width{0}, fb_height{0};
    glfwGetFramebufferSize(&window, &fb_width, &fb_height);

    int win_width{0}, win_height{0};
    glfwGetWindowSize(&window, &win_width, &win_height);

    if (fb_width <= 0 || fb_height <= 0 || win_width <= 0 || win_height <= 0)
    {
        return std::nullopt;
    }

    const auto fb_width_f = static_cast<f32>(fb_width);
    const auto fb_height_f = static_cast<f32>(fb_height);
    const auto win_width_f = static_cast<f32>(win_width);
    const auto win_height_f = static_cast<f32>(win_height);

    const auto scale_x = fb_width_f / win_width_f;
    const auto scale_y = fb_height_f / win_height_f;

    const auto left_px = img_pos_screen.x * scale_x;
    const auto width_px = img_size_screen.x * scale_x;

    // ImGui screen coords: origin top-left; OpenGL framebuffer coords: origin bottom-left.
    const auto bottom_px = fb_height_f - (img_pos_screen.y + img_size_screen.y) * scale_y;
    const auto height_px = img_size_screen.y * scale_y;

    const auto vx = static_cast<int>(std::lround(left_px));
    const auto vw = static_cast<int>(std::lround(width_px));
    const auto vy = static_cast<int>(std::lround(bottom_px));
    const auto vh = static_cast<int>(std::lround(height_px));

    RectInt r{};
    r.x = std::clamp(vx, 0, fb_width - 1);
    r.y = std::clamp(vy, 0, fb_height - 1);
    r.w = std::clamp(vw, 1, fb_width - r.x);
    r.h = std::clamp(vh, 1, fb_height - r.y);
    return r;
}

[[nodiscard]] auto try_entity_body(SimulationContext& sim, const Entity& ent) noexcept
    -> const RigidBody*
{
    if (!ent.body)
    {
        return nullptr;
    }
    return sim.physics.try_body(*ent.body);
}

[[nodiscard]] auto compute_entity_color(GfxContext& gfx, const Entity& ent) noexcept -> Color3
{
    Expects(gfx.engine_context);
    Color3 color{ent.color};

    if (!gfx.phys_debug.enabled)
    {
        return color;
    }

    EngineContext* e = gfx.engine_context;
    if (!e)
    {
        return color;
    }

    const RigidBody* rb = try_entity_body(e->simulation, ent);
    if (!rb || rb->is_static())
    {
        return color;
    }

    using CM = GfxContext::PhysicsDebugSettings::ColorMode;
    switch (gfx.phys_debug.color_mode)
    {
        case CM::Diffuse:
            return color;

        case CM::SleepState:
            {
                auto f_sleep_min = static_cast<f32>(rb->sleep_frames) / 60.0f;
                const auto t = rb->asleep ? 1.0f : std::clamp(f_sleep_min, 0.0f, 1.0f);
                return mix(gfx.phys_debug.sleep_active_color, gfx.phys_debug.sleep_asleep_color, t);
            }

        case CM::KineticEnergy:
            {
                const auto m = 1.0f / rb->inv_mass;
                auto E = 0.5f * m * glm::dot(rb->velocity, rb->velocity);
                if (gfx.phys_debug.ke_include_angular)
                {
                    // TODO: Should cache this
                    const glm::mat3 I_world = glm::inverse(rb->inv_inertia_world);
                    E += 0.5f * glm::dot(rb->angular_velocity, I_world * rb->angular_velocity);
                }

                const auto denom = std::max(1e-6f, gfx.phys_debug.ke_max);
                const auto t = std::clamp(E / denom, 0.0f, 1.0f);

                return mix(gfx.phys_debug.ke_low_color, gfx.phys_debug.ke_high_color, t);
            }
    }

    return color;
}

struct GLStateSnapshot
{
    GLboolean depth_test{};
    GLboolean cull_face{};
    GLboolean stencil_test{};
    GLboolean blend{};
};

[[nodiscard]] auto snapshot_gl_state() noexcept -> GLStateSnapshot
{
    return GLStateSnapshot{
        .depth_test = glIsEnabled(GL_DEPTH_TEST),
        .cull_face = glIsEnabled(GL_CULL_FACE),
        .stencil_test = glIsEnabled(GL_STENCIL_TEST),
        .blend = glIsEnabled(GL_BLEND),
    };
}

auto restore_gl_state(const GLStateSnapshot& s) noexcept -> void
{
    if (s.depth_test)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    if (s.cull_face)
    {
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }

    if (s.stencil_test)
    {
        glEnable(GL_STENCIL_TEST);
    }
    else
    {
        glDisable(GL_STENCIL_TEST);
    }

    if (s.blend)
    {
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

}  // namespace

auto GfxContext::render_to_viewport_objects(
    const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
) const -> void
{
    Expects(engine_context);

    const auto ents = engine_context->simulation.world.entities();
    if (ents.empty())
    {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glStencilMask(0x00);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);

    const auto& prog = shader_programs.obj;
    prog.bind();
    prog.set_uView(camera_view_matrix);
    prog.set_uProj(camera_proj_matrix);

    meshes.cube.vao.bind();
    for (auto i = 0zu; i < ents.size(); ++i)
    {
        const auto& ent = ents[i];
        Expects(ent.id != k_invalid_id);

        if (const auto model = engine_context->simulation.world.model_matrix_at(i))
        {
            prog.set_uModel(*model);
        }
        else
        {
            continue;
        }
        prog.set_uColor(compute_entity_color(*const_cast<GfxContext*>(this), ent));

        glDrawArrays(GL_TRIANGLES, 0, meshes.cube.vertex_count);
    }
    VAO::unbind();
}

auto GfxContext::render_to_viewport_grid(
    const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
) const -> void
{
    glDepthMask(GL_FALSE);

    const auto& prog = shader_programs.grid;
    prog.bind();
    prog.set_uView(camera_view_matrix);
    prog.set_uProj(camera_proj_matrix);
    prog.set_uFogStart(grid.fog_start);
    prog.set_uFogEnd(grid.fog_end);

    meshes.grid.vao.bind();
    glDrawArrays(GL_LINES, 0, meshes.grid.vertex_count);
    VAO::unbind();

    glDepthMask(GL_TRUE);
}

auto GfxContext::render_to_viewport() -> void
{
    Expects(engine_context);
    Expects(viewport_fb_rect_valid && "Should only render to valid viewports");

    glBindFramebuffer(GL_FRAMEBUFFER, viewport_fbo.fbo);
    glViewport(0, 0, viewport_fbo.width, viewport_fbo.height);
    glClearColor(background_color.r(), background_color.g(), background_color.b(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    {
        const auto aspect = viewport_fbo.aspect_ratio();
        const auto& cam = engine_context->simulation.world.editor_state().camera();
        const auto view_matrix = cam.view_matrix();
        const auto proj_matrix = cam.proj_matrix(aspect);

        // clang-format off
        render_to_viewport_grid         (view_matrix, proj_matrix);
        render_to_viewport_objects      (view_matrix, proj_matrix);
        render_to_viewport_outline      (view_matrix, proj_matrix);
        render_to_viewport_physics_debug(view_matrix, proj_matrix);
        render_to_viewport_pivot        (view_matrix, proj_matrix, cam.pivot);
        // clang-format on
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    auto tex_ref = viewport_fbo.imgui_texture_id();
    auto image_size = ImGui::GetContentRegionAvail();

    constexpr ImVec2 uv0{0.0f, 1.0f};
    constexpr ImVec2 uv1{1.0f, 0.0f};

    ImGui::Image(tex_ref, image_size, uv0, uv1);
}

auto GfxContext::render_to_viewport_outline(
    const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
) const -> void
{
    Expects(engine_context);

    const auto& selected = engine_context->simulation.world.editor_state().selected_ids;
    if (selected.empty())
    {
        return;
    }

    for (const EntityId id : selected)
    {
        const Entity* sel = engine_context->simulation.world.find(id);
        if (!sel)
        {
            continue;
        }

        const auto sel_idx = engine_context->simulation.world.find_idx(id);
        if (!sel_idx)
        {
            continue;
        }
        const auto model = engine_context->simulation.world.model_matrix_at(*sel_idx);
        if (!model)
        {
            continue;
        }

        const auto M = ModelMatrix{*model};
        glClear(GL_STENCIL_BUFFER_BIT);

        {  // Pass 1: write stencil (invisible)
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_FALSE);
            glDisable(GL_DEPTH_TEST);

            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

            const auto& prog = shader_programs.obj;
            prog.bind();
            prog.set_uView(camera_view_matrix);
            prog.set_uProj(camera_proj_matrix);
            prog.set_uModel(M);

            meshes.cube.instantiate_once();

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

            const ModelMatrix M_outline{M.m * glm::scale(glm::mat4(1.0f), glm::vec3(1.04f))};

            const auto& prog = shader_programs.outline;
            prog.bind();
            prog.set_uView(camera_view_matrix);
            prog.set_uProj(camera_proj_matrix);
            prog.set_uModel(M_outline);
            prog.set_uColor(k_outline_color);

            meshes.cube.instantiate_once();

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);

            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        }
    }
}

auto GfxContext::render_to_viewport_pivot(
    const ViewMatrix& view_matrix, const ProjMatrix& proj_matrix, const Pos3& pivot
) const -> void
{
    if (!pivot_active)
    {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    const Transform t{.position = pivot, .scale = {0.1f, 0.1f, 0.1f}};

    const auto& prog = shader_programs.pivot;
    prog.bind();
    prog.set_uView(view_matrix);
    prog.set_uProj(proj_matrix);
    prog.set_uModel(t.model_matrix());
    prog.set_uColor(k_pivot_color);

    meshes.sphere.vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, meshes.sphere.vertex_count);
    VAO::unbind();
}

auto GfxContext::viewport_window() -> void
{
    const auto window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
                              | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Viewport", nullptr, window_flags);

    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImVec2 size = ImGui::GetContentRegionAvail();

    viewport_img_pos = {pos.x, pos.y};
    viewport_img_size = {size.x, size.y};

    viewport_fb_rect_valid = false;
    viewport_image_hovered = false;

    if (!window)
    {
        if (!viewport_valid_warning_shown)
        {
            std::println("[Warning] Viewport window not created");
            viewport_valid_warning_shown = true;
        }
        ImGui::End();
        return;
    }

    const auto rect_opt = compute_viewport_fb_rect(*window, viewport_img_pos, viewport_img_size);
    if (!rect_opt)
    {
        if (!viewport_valid_warning_shown)
        {
            std::println("[Warning] Viewport is not valid (empty framebuffer/window)");
            viewport_valid_warning_shown = true;
        }
        ImGui::End();
        return;
    }

    viewport_fb_rect = *rect_opt;

    const int fbo_w = viewport_fb_rect.w;
    const int fbo_h = viewport_fb_rect.h;

    viewport_fb_rect_valid = (fbo_w > 8 && fbo_h > 8) && viewport_fbo.ensure_size(fbo_w, fbo_h);
    if (!viewport_fb_rect_valid)
    {
        if (!viewport_valid_warning_shown)
        {
            std::println("[Warning] Viewport is not valid (too small or FBO resize failed)");
            viewport_valid_warning_shown = true;
        }
        ImGui::End();
        return;
    }

    viewport_valid_warning_shown = false;

    render_to_viewport();
    viewport_image_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    ImGui::End();
}

[[nodiscard]] inline auto max_extent(const Dir3& half_extents) noexcept -> f32
{
    return std::max({half_extents.x, half_extents.y, half_extents.z});
}

auto GfxContext::render_to_viewport_physics_debug(
    const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
) -> void
{
    Expects(engine_context);

    if (!should_render_physics_debug_())
    {
        return;
    }

    debug_line_vertices.clear();

    if (phys_debug.show_contacts)
    {
        append_contact_debug_lines_();
    }

    append_selected_entity_debug_lines_();

    if (debug_line_vertices.empty())
    {
        return;
    }

    ensure_debug_line_mesh_created_();
    upload_debug_lines_to_gpu_();

    const GLStateSnapshot prev = snapshot_gl_state();
    configure_physics_debug_gl_state_();
    draw_debug_lines_(camera_view_matrix, camera_proj_matrix);
    restore_gl_state(prev);
}

[[nodiscard]] auto GfxContext::should_render_physics_debug_() const noexcept -> bool
{
    return phys_debug.enabled && loaded_glad && shader_programs.grid.valid();
}

auto GfxContext::push_debug_line_(const Line3& line, const Color3& color, f32 alpha) noexcept
    -> void
{
    debug_line_vertices.push_back(DebugLineV_PColor{line.a, color, alpha});
    debug_line_vertices.push_back(DebugLineV_PColor{line.b, color, alpha});
}

auto GfxContext::append_contact_debug_lines_() -> void
{
    Expects(engine_context);

    const auto& phys = engine_context->simulation.physics;

    const auto s = std::max(0.0f, phys_debug.contact_marker_size);
    const auto nscale = std::max(0.0f, phys_debug.contact_normal_scale);

    for (const auto& c : phys.debug_contacts)
    {
        const auto alpha = 1.0;
        constexpr auto k_contact_debug_line_color = Color3::White;
        push_debug_line_(
            {c.p + s * k_axis_x, c.p - s * k_axis_x}, k_contact_debug_line_color, alpha
        );
        push_debug_line_(
            {c.p + s * k_axis_y, c.p - s * k_axis_y}, k_contact_debug_line_color, alpha
        );
        push_debug_line_(
            {c.p + s * k_axis_z, c.p - s * k_axis_z}, k_contact_debug_line_color, alpha
        );

        if (phys_debug.show_contact_normals)
        {
            constexpr Color3 k_contact_debug_normal_color{1.0f, 0.75f, 0.1f};
            push_debug_line_({c.p, c.p + c.n * nscale}, k_contact_debug_normal_color, 1.0f);
        }
    }
}

auto GfxContext::append_selected_entity_debug_lines_() -> void
{
    Expects(engine_context);
    const auto& world = engine_context->simulation.world;
    const auto& physics = engine_context->simulation.physics;

    const auto& selected = world.editor_state().selected_ids;
    for (const EntityId id : selected)
    {
        const Entity* entity = world.find(id);
        if (!entity)
        {
            continue;
        }

        const auto* rigid_body = entity->body ? physics.try_body(*entity->body) : nullptr;

        const auto [center_of_mass, ori, extent] = selected_entity_frame_(*entity, rigid_body);

        const auto alpha = 1.0f;
        if (phys_debug.show_selected_axes)
        {
            const auto axis_len = std::max(0.0f, extent) * std::max(0.0f, phys_debug.axis_scale);

            const auto R = glm::mat3_cast(ori);

            const auto ax = axis_len * glm::normalize(R * k_axis_x);
            const auto ay = axis_len * glm::normalize(R * k_axis_y);
            const auto az = axis_len * glm::normalize(R * k_axis_z);

            push_debug_line_({center_of_mass, center_of_mass + ax}, Color3::Red, alpha);
            push_debug_line_({center_of_mass, center_of_mass + ay}, Color3::Green, alpha);
            push_debug_line_({center_of_mass, center_of_mass + az}, Color3::Blue, alpha);
        }

        if (rigid_body)
        {
            {  // Linear Velocity
                const auto velo = rigid_body->velocity;
                const auto v2 = glm::dot(velo, velo);
                if (v2 > 1e-8f)
                {
                    const auto dv = phys_debug.vel_scale * (velo / v2);
                    push_debug_line_({center_of_mass, center_of_mass + dv}, Color3::Orange, alpha);
                }
            }

            {  // Angular Velocity
                const auto ang_velo = rigid_body->angular_velocity;
                const auto av2 = glm::dot(ang_velo, ang_velo);
                if (av2 > 1e-8f)
                {
                    const auto dv = phys_debug.ang_vel_scale * (ang_velo / av2);
                    push_debug_line_({center_of_mass, center_of_mass + dv}, Color3::Cyan, alpha);
                }
            }
        }
    }
}

auto GfxContext::selected_entity_frame_(
    const Entity& entity, const RigidBody* rigid_body
) const noexcept -> std::tuple<Pos3, Quaternion, f32>
{
    if (rigid_body)
    {
        return {
            rigid_body->position,
            rigid_body->orientation,
            2.0f * max_extent(rigid_body->half_extents),
        };
    }

    return {
        entity.transform.position,
        entity.transform.orientation,
        max_extent(entity.transform.scale),
    };
}

auto GfxContext::ensure_debug_line_mesh_created_() -> void
{
    if (debug_line_created)
    {
        return;
    }

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

auto GfxContext::upload_debug_lines_to_gpu_() -> void
{
    meshes.debug_line.vertex_count = static_cast<GLsizei>(debug_line_vertices.size());

    const ScopedBufferBinder binder{meshes.debug_line};
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(debug_line_vertices.size() * sizeof(DebugLineV_PColor)),
        debug_line_vertices.data(),
        GL_DYNAMIC_DRAW
    );
}

auto GfxContext::configure_physics_debug_gl_state_() const noexcept -> void
{
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
}

auto GfxContext::draw_debug_lines_(
    const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
) const -> void
{
    constexpr f32 k_no_fog_start = 1.0e6f;
    constexpr f32 k_no_fog_end = 2.0e6f;

    const auto& prog = shader_programs.grid;
    prog.bind();
    prog.set_uView(camera_view_matrix);
    prog.set_uProj(camera_proj_matrix);
    prog.set_uFogStart(k_no_fog_start);
    prog.set_uFogEnd(k_no_fog_end);

    meshes.debug_line.vao.bind();
    glDrawArrays(GL_LINES, 0, meshes.debug_line.vertex_count);
    VAO::unbind();
}

}  // namespace ds_pba
