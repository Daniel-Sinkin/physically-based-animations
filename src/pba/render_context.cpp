// pba/render_context.cpp
#include "pba/mesh_data.hpp"
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/render_context.hpp"
//
#include "pba/format.hpp"  // IWYU pragma: keep
//
#include "pba/core_types.hpp"
#include "pba/gl.hpp"
#include "pba/gl_types.hpp"
#include "pba/gltf_mesh.hpp"
#include "pba/math_types.hpp"
#include "pba/mesh.hpp"
#include "pba/raycast.hpp"
#include "pba/scene_context.hpp"
#include "pba/scene_types.hpp"
#include "pba/ui.hpp"
#include "pba/util/shutdown.hpp"

#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <imgui.h>
#include <json.hpp>
namespace
{
[[nodiscard]] ds_pba::GLMesh upload_mesh_pn(const ds_pba::MeshData& mesh_data)
{
    using namespace ds_pba;

    const auto& verts = mesh_data.vertices;
    assert(!verts.empty());

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    {
        const ScopedBufferBinder binder{mesh};

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(verts.size() * sizeof(MeshV)),
            verts.data(),
            GL_STATIC_DRAW
        );

        const auto stride = static_cast<GLsizei>(sizeof(MeshV));

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, GLPtr::offset0());

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, GLPtr::offset(3 * sizeof(f32)));
    }

    return mesh;
}
}  // namespace
ds_pba::RenderContext::~RenderContext()
{
    shutdown();
}

bool ds_pba::RenderContext::is_active() const
{
    return (window != nullptr) && !glfwWindowShouldClose(window) && is_active_;
}

void ds_pba::RenderContext::request_close() noexcept
{
    is_active_ = false;
    if (window)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void ds_pba::RenderContext::render_to_viewport_objects(
    const ds_pba::ViewMatrix& camera_view_matrix, const ds_pba::ProjMatrix& camera_proj_matrix
) const
{  // Objects
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glStencilMask(0x00);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);

    obj_prog.bind();
    set_uniform_mat4(obj_prog.id, "uView", camera_view_matrix);
    set_uniform_mat4(obj_prog.id, "uProj", camera_proj_matrix);

    {  // Cubes
        cube_mesh.vao.bind();
        for (usize i{0zu}; i < scene_context->cube_objects.size(); ++i)
        {
            const Object& o{scene_context->cube_objects[i]};
            assert(o.id != k_invalid_id);

            set_uniform_mat4(obj_prog.id, "uModel", o.transform.model_matrix());
            set_uniform_vec3(obj_prog.id, "uColor", o.color);
            glDrawArrays(GL_TRIANGLES, 0, cube_mesh.vertex_count);
        }
        VAO::unbind();
    }

    constexpr const bool render_general_mesh{true};
    if constexpr (!render_general_mesh)
    {  // Spheres
        sphere_mesh.vao.bind();
        for (usize i{0zu}; i < scene_context->sphere_objects.size(); ++i)
        {
            const Object& o{scene_context->sphere_objects[i]};
            assert(o.id != k_invalid_id);

            set_uniform_mat4(obj_prog.id, "uModel", o.transform.model_matrix());
            set_uniform_vec3(obj_prog.id, "uColor", o.color);

            glDrawArrays(GL_TRIANGLES, 0, sphere_mesh.vertex_count);
        }
        for (usize i{0zu}; i < scene_context->hitmarker_objects.size(); ++i)
        {
            const Object& o{scene_context->hitmarker_objects[i]};
            assert(o.id != k_invalid_id);

            set_uniform_mat4(obj_prog.id, "uModel", o.transform.model_matrix());
            set_uniform_vec3(obj_prog.id, "uColor", o.color);

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
        set_uniform_mat4(obj_prog.id, "uModel", o.transform.model_matrix());
        set_uniform_vec3(obj_prog.id, "uColor", o.color);

        glDrawArrays(GL_TRIANGLES, 0, marble_bust_mesh.vertex_count);
        VAO::unbind();
    }
}

void ds_pba::RenderContext::render_to_viewport_outline(
    const ds_pba::ViewMatrix& camera_view_matrix, const ds_pba::ProjMatrix& camera_proj_matrix
) const
{
    if (!scene_context->selected_index)
    {
        return;
    }
    // Outline via stencil (in FBO)
    assert(scene_context->selected_type.has_value());

    auto type = *scene_context->selected_type;
    auto idx = *scene_context->selected_index;

    auto selector = [&](ObjectType type, usize idx) -> const Object&
    {
        switch (type)
        {
            case ds_pba::ObjectType::Cube:
                return scene_context->cube_objects[idx];
            case ds_pba::ObjectType::Sphere:
                return scene_context->sphere_objects[idx];
            case ds_pba::ObjectType::Hitmarker:
                return scene_context->hitmarker_objects[idx];
        }
    };
    const Object& sel = selector(type, idx);

    const glm::mat4 M = sel.transform.model_matrix();

    auto instantiate_selection_type = [&]() -> void
    {
        switch (type)
        {
            case ds_pba::ObjectType::Cube:
                {
                    cube_mesh.instantiate_once();
                    break;
                }
            case ds_pba::ObjectType::Sphere:
            case ds_pba::ObjectType::Hitmarker:
                {
                    sphere_mesh.instantiate_once();
                    break;
                }
        }
    };

    {  // Pass 1: write stencil
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);

        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        obj_prog.bind();
        set_uniform_mat4(obj_prog.id, "uView", camera_view_matrix);
        set_uniform_mat4(obj_prog.id, "uProj", camera_proj_matrix);
        set_uniform_mat4(obj_prog.id, "uModel", M);

        instantiate_selection_type();

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
        set_uniform_mat4(outline_prog.id, "uModel", M_outline);
        set_uniform_mat4(outline_prog.id, "uView", camera_view_matrix);
        set_uniform_mat4(outline_prog.id, "uProj", camera_proj_matrix);
        set_uniform_vec3(outline_prog.id, "uColor", glm::vec3(1.0f, 0.55f, 0.0f));

        instantiate_selection_type();

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    }
}

void ds_pba::RenderContext::render_to_viewport_pivot(
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
    set_uniform_mat4(pivot_prog.id, "uView", camera_view_matrix);
    set_uniform_mat4(pivot_prog.id, "uProj", camera_proj_matrix);

    pivot_mesh.vao.bind();

    const Transform t{.position = pivot_pos, .scale = {0.1f, 0.1f, 0.1f}};
    set_uniform_mat4(obj_prog.id, "uModel", t.model_matrix());
    set_uniform_vec3(obj_prog.id, "uColor", {196.0f / 255.0f, 209.0f / 255.0f, 102.0f / 255.0f});
    glDrawArrays(GL_TRIANGLES, 0, pivot_mesh.vertex_count);

    VAO::unbind();
}

void ds_pba::RenderContext::render_to_viewport_grid(
    const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
) const
{
    // Grid
    glDepthMask(GL_FALSE);

    grid_prog.bind();
    set_uniform_mat4(grid_prog.id, "uView", camera_view_matrix);
    set_uniform_mat4(grid_prog.id, "uProj", camera_proj_matrix);
    set_uniform_float(grid_prog.id, "uFogStart", grid.fog_start);
    set_uniform_float(grid_prog.id, "uFogEnd", grid.fog_end);

    grid_mesh.vao.bind();
    glDrawArrays(GL_LINES, 0, grid_mesh.vertex_count);
    VAO::unbind();

    glDepthMask(GL_TRUE);
}

void ds_pba::RenderContext::render_to_viewport() const
{
    assert(viewport_fb_rect_valid && "Should only render to valid viewports");
    const ImVec2 content_size = ImGui::GetContentRegionAvail();

    glBindFramebuffer(GL_FRAMEBUFFER, viewport_fbo.fbo);
    glViewport(0, 0, viewport_fbo.width, viewport_fbo.height);

    auto bg = background_color;
    glClearColor(bg.r(), bg.g(), bg.b(), bg.a());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    f32 aspect = viewport_fbo.aspect_ratio();

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

void ds_pba::RenderContext::viewport_window()
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

    int win_w = 1, win_h = 1;
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

        const int fbo_w = viewport_fb_rect.width;
        const int fbo_h = viewport_fb_rect.height;
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
        if (!viewport_valid_warning_shown)
        {
            std::println("[Warning] Viewport is not valid!");
            viewport_valid_warning_shown = true;
            assert(
                !viewport_image_hovered
                && "If viewport was already invalid then hovered must have been set to false"
            );
        }
        viewport_image_hovered = false;
    }
    ImGui::End();
}

void ds_pba::RenderContext::hover_interaction_selection(const Raycast& rc) const
{
    bool found{false};
    if (rc.object_type == ObjectType::Cube)
    {
        for (usize i{0zu}; i < scene_context->cube_objects.size(); ++i)
        {
            const Object& obj = scene_context->cube_objects[i];
            if (obj.id == rc.object_id)
            {
                if (scene_context->selected_index == i)
                {  // Deselect on selecting again
                    scene_context->selected_index = std::nullopt;
                    scene_context->selected_type = std::nullopt;
                }
                else
                {
                    scene_context->selected_index = i;
                    scene_context->selected_type = ObjectType::Cube;
                }
                std::println();
                ui_log(std::format("Selected Cube [id={}]", obj.id));
                found = true;
                break;
            }
        }
    }
    if (!found && rc.object_type == ObjectType::Sphere)
    {
        for (usize i{0zu}; i < scene_context->sphere_objects.size(); ++i)
        {
            const Object& obj = scene_context->sphere_objects[i];
            if (obj.id == rc.object_id)
            {
                if (scene_context->selected_index == i)
                {  // Deselect on selecting again
                    scene_context->selected_index = std::nullopt;
                    scene_context->selected_type = std::nullopt;
                }
                else
                {
                    scene_context->selected_index = i;
                    scene_context->selected_type = ObjectType::Sphere;
                }
                ui_log(std::format("Selected Sphere [id={}]", obj.id));
                found = true;
                break;
            }
        }
    }
    if constexpr (false)
    {  // Selection for Hitmarker is disabled
        if (!found && rc.object_type == ObjectType::Hitmarker)
        {
            for (usize i{0zu}; i < scene_context->hitmarker_objects.size(); ++i)
            {
                const Object& obj = scene_context->hitmarker_objects[i];
                if (obj.id == rc.object_id)
                {
                    scene_context->selected_index = i;
                    scene_context->selected_type = ObjectType::Hitmarker;
                    ui_log(std::format("Selected Hitmarker [id={}]", obj.id));
                    found = true;
                    break;
                }
            }
        }
    }
}

void ds_pba::RenderContext::hover_interaction_holding_middle(
    f64 mouse_x, f64 mouse_y, Camera& cam
) const
{
    // Holding Middle mouse button
    const f32 dx = static_cast<f32>(mouse_x - prev_mx);
    const f32 dy = static_cast<f32>(mouse_y - prev_my);

    const bool left_shift_down = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    const bool right_shift_down = glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    if (left_shift_down || right_shift_down)
    {  // Move Pivot
        const f32 vp_h = std::max(1.0f, viewport_img_size.y);

        const f32 units_per_px = (2.0f * cam.distance * std::tan(0.5f * cam.fov_y)) / vp_h;

        auto right_offset = (-dx * units_per_px) * cam.right();
        auto up_offset = dy * units_per_px * cam.up();
        cam.pivot += (right_offset + up_offset) * pan_sensitivity;
    }
    else
    {  // Rotate Around pivot
        scene_context->camera.yaw += -dx * sensitivity;
        scene_context->camera.pitch += dy * sensitivity;

        const f32 lim = glm::radians(89.0f);
        scene_context->camera.pitch = std::clamp(scene_context->camera.pitch, -lim, lim);
    }
}

void ds_pba::RenderContext::hover_interaction(
    f64 mouse_x, f64 mouse_y, bool left_down, bool middle_down, bool right_down
) const
{
    const ImGuiIO& io = ImGui::GetIO();

    Camera& cam{scene_context->camera};
    assert(viewport_fb_rect_valid && "If viewport hovered then it must be valid");
    const f32 wheel = io.MouseWheel;
    if (wheel != 0.0f)
    {  // Zooming
        cam.distance *= std::exp(-wheel * zoom_speed);
        cam.distance = std::clamp(scene_context->camera.distance, 0.75f, 200.0f);
    }

    if (middle_down && prev_middle)
    {
        hover_interaction_holding_middle(mouse_x, mouse_y, cam);
    }

    const bool selecting{left_down && !prev_left};
    const bool spawning{right_down && !prev_right};
    if (selecting || spawning)
    {  // Selecting objects
        const f32 aspect = viewport_fbo.aspect_ratio();
        const glm::mat4 camera_view_matrix = scene_context->camera.view_matrix();
        const glm::mat4 camera_proj_matrix = scene_context->camera.proj_matrix(aspect);

        const glm::vec2 mouse_pos{glm::vec2{static_cast<f32>(mouse_x), static_cast<f32>(mouse_y)}};

        const Ray mouse_ray = ray_from_imgui_rect(
            mouse_pos, viewport_img_pos, viewport_img_size, camera_view_matrix, camera_proj_matrix
        );

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
                ui_log(
                    std::format(
                        "Hit Object [id={}] at {} [distance from camera {:.2f}]",
                        rc.object_id,
                        rc.hit,
                        rc.t
                    )
                );
                scene_context->hitmarker_objects.push_back(
                    Object{
                        .id = next_object_id(),
                        .type = ObjectType::Sphere,
                        .transform = {.position = rc.hit, .scale = {0.05f, 0.05f, 0.05f}},
                        .color = {1.0f, 1.0f, 1.0f},
                    }
                );
            }
        }
    }
}

void ds_pba::RenderContext::step()
{
    using namespace ds_pba;
    assert(scene_context && "Scene Context not set for RenderContext");

    const ImGuiIO& io = ImGui::GetIO();
    if (!is_active())
    {
        return;
    }
    if (ds_pba::g_request_close.load(std::memory_order_relaxed))
    {
        request_close();
    }
    glfwPollEvents();

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

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

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

    const auto mouse_x = static_cast<f64>(io.MousePos.x);
    const auto mouse_y = static_cast<f64>(io.MousePos.y);

    const bool left_down{glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS};
    const bool middle_down{glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS};
    const bool right_down{glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS};

    if (viewport_image_hovered)
    {
        hover_interaction(mouse_x, mouse_y, left_down, middle_down, right_down);
    }

    prev_left = left_down;
    prev_middle = middle_down;
    prev_right = right_down;
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

    glfwSwapBuffers(window);
    ++frame_count;
}

bool ds_pba::RenderContext::create_programs()
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

    auto pivot_prog_res = create_program_from_file("pivot");
    if (!pivot_prog_res)
    {
        std::println(
            stderr,
            "Failed to load 'pivot' shaders, got error code: {}",
            static_cast<int>(outline_prog_res.error())
        );
        return false;
    }
    pivot_prog = *pivot_prog_res;

    if (!grid_prog.valid() || !obj_prog.valid() || !outline_prog.valid() || !pivot_prog.valid())
    {
        std::println(stderr, "Failed to create shader programs");
        return false;
    }

    return true;
}

bool ds_pba::RenderContext::create_meshes()
{
    cube_mesh = upload_mesh_pn(create_cube_mesh());
    sphere_mesh = upload_mesh_pn(create_sphere_mesh(32, 24, 1.0f));

    grid_mesh = create_grid_mesh(grid);  // TODO: Change away from GL type

    cylinder_mesh = upload_mesh_pn(create_cylinder_mesh(24, 0.5f, 1.0f));
    pyramid_mesh = upload_mesh_pn(create_pyramid_mesh());

    {
        auto mesh_res = ds_pba::load_model_mesh("marble_bust_01");
        assert(mesh_res && "Failed to load marble bust mesh");
        marble_bust_mesh = upload_mesh_pn(*mesh_res);
    }

    return true;
}

bool ds_pba::RenderContext::setup()
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

            const int ww = std::max(1, static_cast<int>(std::lround(desired_fb_w / sx)));
            const int wh = std::max(1, static_cast<int>(std::lround(desired_fb_h / sy)));

            glfwSetWindowSize(window, ww, wh);

            const int wx{monitor_x + (mode->width - ww) / 2};
            const int wy{monitor_y + (mode->height - wh) / 2};

            glfwSetWindowPos(window, wx, wy);
        }
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
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

    const char* glsl_version = "#version 330";
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

void ds_pba::RenderContext::shutdown()
{
    if (loaded_glad)
    {
        viewport_fbo.destroy();

        glDeleteProgram(grid_prog.id);
        glDeleteProgram(obj_prog.id);
        glDeleteProgram(outline_prog.id);
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
