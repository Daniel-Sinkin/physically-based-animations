// pba/render/render_context.cpp
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
#include "pba/core/math_types.hpp"
#include "pba/engine/engine_context.hpp"
#include "pba/engine/scene_context.hpp"
#include "pba/engine/scene_types.hpp"
#include "pba/gfx/gl.hpp"
#include "pba/gfx/gl_types.hpp"
#include "pba/gfx/raycast.hpp"
#include "pba/gfx/video_recorder.hpp"
#include "pba/ui/ui.hpp"
#include "pba/util/shutdown.hpp"
//
#include <atomic>
#include <imgui.h>
#include <optional>
#include <print>
#include <utility>
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <json.hpp>

namespace
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
}  // namespace

ds_pba::GfxContext::~GfxContext()
{
    shutdown();
}

bool ds_pba::GfxContext::is_active() const
{
    return (window != nullptr) && !glfwWindowShouldClose(window) && is_active_;
}

void ds_pba::GfxContext::request_close() noexcept
{
    is_active_ = false;
    if (window)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void ds_pba::GfxContext::render_to_viewport_objects(
    const ds_pba::ViewMatrix& camera_view_matrix, const ds_pba::ProjMatrix& camera_proj_matrix
) const
{  // Objects
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

void ds_pba::GfxContext::render_to_viewport_outline(
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

void ds_pba::GfxContext::render_to_viewport_pivot(
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

void ds_pba::GfxContext::render_to_viewport_grid(
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

void ds_pba::GfxContext::render_to_viewport() const
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

void ds_pba::GfxContext::viewport_window()
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

void ds_pba::GfxContext::hover_interaction_selection(const Raycast& rc) const
{
    assert(scene_context);

    const bool left_shift_down = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    const bool right_shift_down = glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    const bool shift_down = left_shift_down || right_shift_down;

    auto log_action = [&](std::string_view action, ObjectId id, const char* kind) -> void
    {
        if (engine_context)
        {
            if (auto it = engine_context->obj_name_map.find(id);
                it != engine_context->obj_name_map.end())
            {
                ui_log(std::format("{} {} [id={}] [{}]", action, it->second, id, kind));
                return;
            }
        }
        ui_log(std::format("{} [id={}] [{}]", action, id, kind));
    };

    const char* kind = "";
    switch (rc.object_type)
    {
        case ObjectType::Cube:
            kind = "Cube";
            break;
        case ObjectType::Sphere:
            kind = "Sphere";
            break;
        case ObjectType::Hitmarker:
            kind = "Hitmarker";
            break;
    }

    const bool was_selected = scene_context->is_selected(rc.object_id);

    if (shift_down)
    {
        scene_context->toggle_selection(rc.object_id);
    }
    else
    {
        scene_context->select_single(rc.object_id);
    }

    const bool now_selected = scene_context->is_selected(rc.object_id);
    if (now_selected && !was_selected)
    {
        log_action("Selected", rc.object_id, kind);
    }
    else if (!now_selected && was_selected)
    {
        log_action("Deselected", rc.object_id, kind);
    }
    else
    {
        // e.g. clicking already selected without shift -> still selected
        log_action("Selected", rc.object_id, kind);
    }
}

void ds_pba::GfxContext::hover_interaction_holding_middle(f64 mouse_x, f64 mouse_y, Camera& cam) const
{
    const auto dx = static_cast<f32>(mouse_x - prev_mx);
    const auto dy = static_cast<f32>(mouse_y - prev_my);

    const bool left_shift_down = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    const bool right_shift_down = glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    if (left_shift_down || right_shift_down)
    {  // Move Pivot
        const f32 vp_h{std::max(1.0f, viewport_img_size.y)};

        const f32 units_per_px{(2.0f * cam.distance * std::tan(0.5f * cam.fov_y)) / vp_h};

        auto right_offset = (-dx * units_per_px) * cam.right();
        auto up_offset = dy * units_per_px * cam.up();
        cam.pivot += (right_offset + up_offset) * k_pan_sensitivity;
    }
    else
    {  // Rotate Around pivot
        scene_context->camera.yaw += -dx * k_sensitivity;
        scene_context->camera.pitch += dy * k_sensitivity;

        const f32 lim{glm::radians(89.0f)};
        scene_context->camera.pitch = std::clamp(scene_context->camera.pitch, -lim, lim);
    }
}

void ds_pba::GfxContext::hover_interaction(
    f64 mouse_x, f64 mouse_y, bool left_down, bool middle_down, bool right_down
) const
{
    const ImGuiIO& io{ImGui::GetIO()};

    Camera& cam{scene_context->camera};
    assert(viewport_fb_rect_valid && "If viewport hovered then it must be valid");
    const f32 wheel{io.MouseWheel};
    if (wheel != 0.0f)
    {  // Zooming
        cam.distance *= std::exp(-wheel * k_zoom_speed);
        cam.distance = std::clamp(scene_context->camera.distance, 0.75f, 200.0f);
    }

    if (middle_down)
    {
        hover_interaction_holding_middle(mouse_x, mouse_y, cam);
    }

    const bool selecting{left_down && !prev_left};
    const bool spawning{right_down && !prev_right};
    if (selecting || spawning)
    {  // Selecting objects
        const f32 aspect{viewport_fbo.aspect_ratio()};
        const ViewMatrix camera_view_matrix{scene_context->camera.view_matrix()};
        const ProjMatrix camera_proj_matrix{scene_context->camera.proj_matrix(aspect)};

        const glm::vec2 mouse_pos{glm::vec2{static_cast<f32>(mouse_x), static_cast<f32>(mouse_y)}};

        const Ray mouse_ray{ray_from_imgui_rect(
            mouse_pos, viewport_img_pos, viewport_img_size, camera_view_matrix, camera_proj_matrix
        )};

        const bool left_shift_down = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        const bool right_shift_down = glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        const bool shift_down{left_shift_down || right_shift_down};

        auto rc_res = raycast(*scene_context, mouse_ray);
        if (rc_res)
        {
            const Raycast rc = *rc_res;
            if (selecting)
            {
                hover_interaction_selection(rc);
            }
            if (spawning)
            {
                {
                    auto it = engine_context->obj_name_map.find(rc.object_id);
                    if (it != engine_context->obj_name_map.end())
                    {
                        ui_log(
                            std::format(
                                "Hit Object [id={} {}] at {} [distance from camera {:.2f}]",
                                rc.object_id,
                                it->second,
                                rc.hit,
                                rc.t
                            )
                        );
                    }
                    else
                    {
                        ui_log(
                            std::format(
                                "Hit Object [id={}] at {} [distance from camera {:.2f}]",
                                rc.object_id,
                                rc.hit,
                                rc.t
                            )
                        );
                    }
                }
                scene_context->hitmarker_objects.push_back(
                    Object{
                        .id = next_object_id(),
                        .type = ObjectType::Hitmarker,
                        .transform = {.position = rc.hit, .scale = {0.05f, 0.05f, 0.05f}},
                        .color = {1.0f, 1.0f, 1.0f},
                    }
                );
            }
        }
        else if (selecting && !shift_down)
        {
            // Deselect on clicking on background
            if (!scene_context->selected_ids.empty())
            {

                scene_context->clear_selection();
                ui_log("Deselected all");
            }
        }
    }
}

std::optional<ds_pba::Position3> ds_pba::GfxContext::get_object_position(ObjectId id) const
{
    if (!scene_context)
    {
        return std::nullopt;
    }

    if (engine_context)
    {
        if (auto it = engine_context->obj_map.find(id); it != engine_context->obj_map.end())
        {
            const auto [scene_i, phys_i] = it->second;
            if (phys_i < engine_context->physics.bodies.size())
            {
                return engine_context->physics.bodies[phys_i].position;
            }
            if (scene_i < scene_context->cube_objects.size())
            {
                return scene_context->cube_objects[scene_i].transform.position;
            }
        }
    }

    for (const Object& o : scene_context->cube_objects)
    {
        if (o.id == id)
        {
            return o.transform.position;
        }
    }
    for (const Object& o : scene_context->sphere_objects)
    {
        if (o.id == id)
        {
            return o.transform.position;
        }
    }
    for (const Object& o : scene_context->hitmarker_objects)
    {
        if (o.id == id)
        {
            return o.transform.position;
        }
    }

    return std::nullopt;
}
void ds_pba::GfxContext::set_grab_constraint(GrabConstraint c)
{
    grab.constraint = c;

    switch (c)
    {
        case GrabConstraint::None:
            ui_log("Grab: unconstrained");
            break;
        case GrabConstraint::X:
            ui_log("Grab: constrain X");
            break;
        case GrabConstraint::Y:
            ui_log("Grab: constrain Y");
            break;
        case GrabConstraint::Z:
            ui_log("Grab: constrain Z");
            break;
    }
}

void ds_pba::GfxContext::begin_grab(f64 mouse_x, f64 mouse_y)
{
    assert(scene_context);

    if (scene_context->selected_ids.empty())
    {
        ui_log("Grab (G): nothing selected");
        return;
    }

    grab.active = true;
    grab.start_mouse_x = mouse_x;
    grab.start_mouse_y = mouse_y;
    grab.constraint = GrabConstraint::None;

    grab.start_positions.clear();
    grab.start_positions.reserve(scene_context->selected_ids.size());

    for (const ObjectId id : scene_context->selected_ids)
    {
        if (auto p = get_object_position(id))
        {
            grab.start_positions.emplace_back(id, *p);
        }
    }

    ui_log("Grab: move mouse. X/Y/Z constrain. LMB/Enter confirm. RMB/Esc cancel.");
}

void ds_pba::GfxContext::update_grab(f64 mouse_x, f64 mouse_y)
{
    assert(scene_context);

    if (!grab.active)
    {
        return;
    }

    const Camera& cam{scene_context->camera};

    const f32 vp_h{std::max(1.0f, viewport_img_size.y)};
    const f32 units_per_px{(2.0f * cam.distance * std::tan(0.5f * cam.fov_y)) / vp_h};

    const auto dx = static_cast<f32>(mouse_x - grab.start_mouse_x);
    const auto dy = static_cast<f32>(mouse_y - grab.start_mouse_y);

    // Mouse up = Look down
    const Direction3 v = (dx * units_per_px) * cam.right() + (-dy * units_per_px) * cam.up();

    Direction3 delta = v;

    switch (grab.constraint)
    {
        case GrabConstraint::None:
            break;

        case GrabConstraint::X:
            delta = Direction3{delta.x, 0.0f, 0.0f};
            break;

        case GrabConstraint::Y:
            delta = Direction3{0.0f, delta.y, 0.0f};
            break;

        case GrabConstraint::Z:
            delta = Direction3{0.0f, 0.0f, delta.z};
            break;
    }

    for (const auto& [id, start_pos] : grab.start_positions)
    {
        set_object_position(id, start_pos + delta);
    }
}

void ds_pba::GfxContext::cancel_grab()
{
    if (!grab.active)
    {
        return;
    }

    for (const auto& [id, start_pos] : grab.start_positions)
    {
        set_object_position(id, start_pos);
    }

    grab.active = false;
    grab.start_positions.clear();
    grab.constraint = GrabConstraint::None;
    ui_log("Grab cancelled");
}

void ds_pba::GfxContext::confirm_grab()
{
    if (!grab.active)
    {
        return;
    }

    grab.active = false;
    grab.start_positions.clear();
    grab.constraint = GrabConstraint::None;
    ui_log("Grab confirmed");
}
void ds_pba::GfxContext::set_object_position(ObjectId id, const Position3& p) const
{
    if (!scene_context)
    {
        return;
    }

    // Physics-backed cubes
    if (engine_context)
    {
        if (auto it = engine_context->obj_map.find(id); it != engine_context->obj_map.end())
        {
            const auto [scene_i, phys_i] = it->second;

            if (phys_i < engine_context->physics.bodies.size())
            {
                RigidBody& rb = engine_context->physics.bodies[phys_i];
                rb.position = p;

                rb.velocity = Direction3{};
                rb.angular_velocity = Direction3{};
                rb.force_accum = Direction3{};
                rb.torque_accum = Direction3{};

                rb.asleep = false;
                rb.sleep_frames = 0;
            }

            if (scene_i < scene_context->cube_objects.size())
            {
                scene_context->cube_objects[scene_i].transform.position = p;
            }
            return;
        }
    }

    // Non-physics objects (spheres/hitmarkers)
    for (Object& o : scene_context->sphere_objects)
    {
        if (o.id == id)
        {
            o.transform.position = p;
            return;
        }
    }
    for (Object& o : scene_context->hitmarker_objects)
    {
        if (o.id == id)
        {
            o.transform.position = p;
            return;
        }
    }
}

void ds_pba::GfxContext::step()
{
    using namespace ds_pba;
    assert(scene_context && "Scene Context not set for RenderContext");

    if (!is_active())
    {
        return;
    }
    {  // See shutdown.hpp for details on our signal handling
        if (ds_pba::g_request_close_sig)
        {
            ds_pba::g_request_close.store(true, std::memory_order_relaxed);
            ds_pba::g_request_close_sig = 0;
        }
        if (ds_pba::g_request_close.load(std::memory_order_relaxed))
        {
            request_close();
        }
    }

    glfwPollEvents();

    const ImGuiIO& io{ImGui::GetIO()};
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar())
    {
        render_menu_bar(*this);
        ImGui::EndMainMenuBar();
    }

    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    viewport_window();

    const bool f1_down{glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS};
    if (f1_down && !prev_f1)
    {  // Switch pivot off and on
        if (pivot_active)
        {
            ui_log("Deactivated the pivot");
            pivot_active = false;
        }
        else
        {
            ui_log("Activated the pivot");
            pivot_active = true;
        }
    }
    prev_f1 = f1_down;

    const bool f2_down{glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS};
    if (f2_down && !prev_f2)
    {
        toggle_recording();
    }
    prev_f2 = f2_down;

    const auto mouse_x = static_cast<f64>(io.MousePos.x);
    const auto mouse_y = static_cast<f64>(io.MousePos.y);

    const bool left_down{glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS};
    const bool middle_down{glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS};
    const bool right_down{glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS};
    const bool g_down{glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS};
    const bool esc_down{glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS};
    const bool enter_down{glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS};
    const bool kp_enter_down{glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS};

    const bool g_pressed{g_down && !prev_g};

    const bool esc_pressed{esc_down && !prev_esc};
    const bool enter_pressed{enter_down && !prev_enter};
    const bool kp_enter_pressed{kp_enter_down && !prev_kp_enter};

    // Grab mode takes priority
    if (grab.active)
    {
        update_grab(mouse_x, mouse_y);
        if (ImGui::IsKeyPressed(ImGuiKey_X))
        {
            set_grab_constraint(GrabConstraint::X);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Y))
        {
            set_grab_constraint(GrabConstraint::Y);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            set_grab_constraint(GrabConstraint::Z);
        }

        const bool confirm{(left_down && !prev_left) || enter_pressed || kp_enter_pressed};
        const bool cancel{(right_down && !prev_right) || esc_pressed};

        if (cancel)
        {
            cancel_grab();
        }
        else if (confirm)
        {
            confirm_grab();
        }
    }
    else
    {
        if (viewport_image_hovered && g_pressed && !io.WantCaptureKeyboard)
        {
            begin_grab(mouse_x, mouse_y);
        }

        if (esc_pressed)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        if (viewport_image_hovered)
        {
            hover_interaction(mouse_x, mouse_y, left_down, middle_down, right_down);
        }
    }

    prev_left = left_down;
    prev_middle = middle_down;
    prev_right = right_down;

    prev_g = g_down;

    prev_esc = esc_down;
    prev_enter = enter_down;
    prev_kp_enter = kp_enter_down;

    prev_mx = mouse_x;
    prev_my = mouse_y;

    render_imgui_windows(*engine_context);

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    if (recorder.is_recording())
    {
        const int w{viewport_fbo.width};
        const int h{viewport_fbo.height};

        if (!viewport_fb_rect_valid)
        {
            ui_log("Recording stopped: viewport became invalid");
            stop_recording();
        }
        else if (w != recorder.width || h != recorder.height)
        {
            ui_log("Recording stopped: viewport size changed during recording");
            stop_recording();
        }
        else
        {
            if (!capture_viewport_rgba8(capture_rgba))
            {
                ui_log("Recording stopped: framebuffer readback failed");
                stop_recording();
            }
            else
            {
                const std::span<const u8> frame{capture_rgba.data(), capture_rgba.size()};
                if (!recorder.write_frame(frame))
                {
                    ui_log("Recording stopped: failed to write frame to ffmpeg");
                    stop_recording();
                }
            }
        }
    }
    glfwSwapBuffers(window);
    ++frame_count;
}

bool ds_pba::GfxContext::create_programs()
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

bool ds_pba::GfxContext::create_meshes()
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

bool ds_pba::GfxContext::setup()
{
    using namespace ds_pba;

    auto glfw_error_callback = [](int error, const char* description)
    { std::println(stderr, "GLFW Error {}: {}", error, description); };

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        std::println(stderr, "Failed to init glfw");
        return false;
    }
    initialised_glfw = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    // No focus on startup
    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);

    window = glfwCreateWindow(1600, 900, "Physically Based Animations", nullptr, nullptr);
    if (!window)
    {
        std::println(stderr, "Failed to create window");
        return false;
    }
    window_created = true;
    {  // Place on 2nd monitor with correct sizing for mixed-DPI
        int monitor_count{0};
        GLFWmonitor* const* monitors = glfwGetMonitors(&monitor_count);

        if (monitor_count >= 2)
        {
            GLFWmonitor* monitor{monitors[1]};

            int monitor_x{0};
            int monitor_y{0};
            glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);

            const GLFWvidmode* mode{glfwGetVideoMode(monitor)};

            const int desired_fb_w{2400};
            const int desired_fb_h{1350};
            float sx{1.0f};
            f32 sy{1.0f};
            glfwGetMonitorContentScale(monitor, &sx, &sy);

            const int window_width{std::max(1, static_cast<int>(std::lround(desired_fb_w / sx)))};
            const int window_height{std::max(1, static_cast<int>(std::lround(desired_fb_h / sy)))};

            glfwSetWindowSize(window, window_width, window_height);

            const int window_x{monitor_x + (mode->width - window_width) / 2};
            const int window_y{monitor_y + (mode->height - window_height) / 2};

            glfwSetWindowPos(window, window_x, window_y);
        }
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::println(stderr, "Failed to init glad");
        shutdown();
        return false;
    }
    loaded_glad = true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    initialised_imgui = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    {  // Load Fonts
        default_font = io.Fonts->AddFontDefault();

        fonts_by_id["default"] = default_font;

        if (ImFont* f =
                io.Fonts->AddFontFromFileTTF("assets/fonts/MonaspaceKrypton-Regular.otf", 14.0f))
        {
            fonts_by_id["krypton-14"] = f;
        }

        if (ImFont* f =
                io.Fonts->AddFontFromFileTTF("assets/fonts/MonaspaceArgon-Regular.otf", 14.0f))
        {
            fonts_by_id["argon-14"] = f;
        }
    }
    {  // Load Themes
        auto res = ui_theme::load_theme_pack_json("assets/ui/themes.json");
        if (res)
        {
            theme_pack = std::move(*res);
            theme_loaded = true;

            theme_index = theme_pack.default_index.value_or(0zu);
            theme_index = std::min(theme_index, theme_pack.themes.size() - 1zu);

            ui_theme::apply_theme(theme_pack.themes[theme_index]);
        }
        else
        {
            ImGui::StyleColorsDark();
        }
    }

    const char* glsl_version{"#version 330"};
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
    {
        std::println(stderr, "ImGui_ImplGlfw_InitForOpenGL failed");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version))
    {
        std::println(stderr, "ImGui_ImplOpenGL3_Init failed");
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    if (!create_programs())
    {
        return false;
    }
    if (!create_meshes())
    {
        return false;
    }
    last_scene_poll = std::chrono::steady_clock::now();
    return true;
}

bool ds_pba::GfxContext::capture_viewport_rgba8(std::vector<u8>& out) const
{
    if (!loaded_glad)
    {
        return false;
    }
    if (!viewport_fb_rect_valid)
    {
        return false;
    }
    if (viewport_fbo.fbo == 0 || viewport_fbo.color_tex == 0)
    {
        return false;
    }

    const int w{viewport_fbo.width};
    const int h{viewport_fbo.height};
    if (w <= 0 || h <= 0)
    {
        return false;
    }

    const usize bytes = static_cast<usize>(w) * static_cast<usize>(h) * 4zu;
    out.resize(bytes);

    GLint prev_fbo{0};
    GLint prev_read_buffer{0};
    GLint prev_pack_alignment{0};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    glGetIntegerv(GL_READ_BUFFER, &prev_read_buffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &prev_pack_alignment);

    glBindFramebuffer(GL_FRAMEBUFFER, viewport_fbo.fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, out.data());

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prev_fbo));
    glReadBuffer(static_cast<GLenum>(prev_read_buffer));
    glPixelStorei(GL_PACK_ALIGNMENT, prev_pack_alignment);

    const GLenum err = glGetError();
    return err == GL_NO_ERROR;
}

void ds_pba::GfxContext::start_recording()
{
    if (recorder.is_recording())
    {
        return;
    }

    if (!viewport_fb_rect_valid)
    {
        ui_log("Recording start failed: viewport is not valid");
        return;
    }

    const int w{viewport_fbo.width};
    const int h{viewport_fbo.height};
    if (w <= 0 || h <= 0)
    {
        ui_log("Recording start failed: invalid viewport size");
        return;
    }

    std::filesystem::path out_dir = capture_output_dir;
    if (out_dir.empty())
    {
        out_dir = "renders";
    }

    std::filesystem::path out_path{};
    try
    {
        std::filesystem::create_directories(out_dir);
        const std::string file = std::format("capture_{:04}.mp4", capture_take_index++);
        out_path = out_dir / file;
    }
    catch (const std::exception& e)
    {
        ui_log(std::format("Recording start failed: {}", e.what()));
        return;
    }

    recorder.width = w;
    recorder.height = h;
    recorder.fps = capture_fps;
    if (!recorder.start(out_path))
    {
        ui_log("Recording start failed: could not start ffmpeg");
        return;
    }
    ui_log(
        std::format("Recording started: {} ({}x{} @ {} fps)", out_path.string(), w, h, capture_fps)
    );
}

void ds_pba::GfxContext::stop_recording()
{
    if (!recorder.is_recording())
    {
        return;
    }

    const std::string out = recorder.output_path.string();
    recorder.stop();
    ui_log(std::format("Recording stopped: {} ({} frames)", out, recorder.frames_written));
}

void ds_pba::GfxContext::toggle_recording()
{
    if (recorder.is_recording())
    {
        stop_recording();
    }
    else
    {
        start_recording();
    }
}

void ds_pba::GfxContext::shutdown()
{
    if (recorder.is_recording())
    {
        recorder.stop();
    }

    if (loaded_glad)
    {
        if (window)
        {
            glfwMakeContextCurrent(window);
        }

        viewport_fbo.destroy();

        invalidate_uniform_cache_for_program(grid_prog.handle());
        invalidate_uniform_cache_for_program(obj_prog.handle());
        invalidate_uniform_cache_for_program(outline_prog.handle());
        invalidate_uniform_cache_for_program(pivot_prog.handle());

        glDeleteProgram(grid_prog.id);
        glDeleteProgram(obj_prog.id);
        glDeleteProgram(outline_prog.id);
        glDeleteProgram(pivot_prog.id);

        grid_prog.id = 0;
        obj_prog.id = 0;
        outline_prog.id = 0;
        pivot_prog.id = 0;

        auto destroy_mesh = [](GLMesh& m)
        {
            if (m.vbo.id != 0)
            {
                glDeleteBuffers(1, &m.vbo.id);
                m.vbo.id = 0;
            }
            if (m.vao.id != 0)
            {
                glDeleteVertexArrays(1, &m.vao.id);
                m.vao.id = 0;
            }
            m.vertex_count = 0;
        };

        destroy_mesh(cube_mesh);
        destroy_mesh(sphere_mesh);
        destroy_mesh(grid_mesh);
        destroy_mesh(marble_bust_mesh);
        destroy_mesh(pyramid_mesh);
        destroy_mesh(cylinder_mesh);

        loaded_glad = false;
    }
    if (initialised_imgui)
    {

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        initialised_imgui = false;
    }
    if (window_created && window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
        window_created = false;
    }
    if (initialised_glfw)
    {
        glfwTerminate();
        initialised_glfw = false;
    }
}
