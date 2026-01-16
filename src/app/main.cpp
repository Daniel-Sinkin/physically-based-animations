// pba/main.cpp
#include "glm/fwd.hpp"
#include "pba/camera.hpp"
#include "pba/gl.hpp"
#include "pba/glfw_setup.hpp"
#include "pba/interaction.hpp"
#include "pba/mesh.hpp"
#include "pba/render_settings.hpp"
#include "pba/scene_context.hpp"
#include "pba/types.hpp"
#include "pba/ui.hpp"
#include "pba/viewport_fbo.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <print>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

int main() {
    using namespace ds_pba;

    auto window_res = setup_glfw();
    if (!window_res) {
        std::println(stderr, "Failed to setup glfw with error code: {}", static_cast<int>(window_res.error()));
        return EXIT_FAILURE;
    }
    GLFWwindow *window = *window_res;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    apply_blender_style();

    const char *glsl_version = "#version 330";
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::println(stderr, "ImGui_ImplGlfw_InitForOpenGL failed");
        return EXIT_FAILURE;
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        std::println(stderr, "ImGui_ImplOpenGL3_Init failed");
        return EXIT_FAILURE;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    auto grid_prog_res = create_program_from_file("grid");
    if (!grid_prog_res) {
        std::println(stderr, "Failed to load 'grid' shaders, got error code: {}", static_cast<int>(grid_prog_res.error()));
        return EXIT_FAILURE;
    }
    ShaderProgram grid_prog = *grid_prog_res;

    auto obj_prog_res = create_program_from_file("object");
    if (!obj_prog_res) {
        std::println(stderr, "Failed to load 'object' shaders, got error code: {}", static_cast<int>(obj_prog_res.error()));
        return EXIT_FAILURE;
    }
    ShaderProgram obj_prog = *obj_prog_res;

    auto outline_prog_res = create_program_from_file("outline");
    if (!outline_prog_res) {
        std::println(stderr, "Failed to load 'outline' shaders, got error code: {}", static_cast<int>(outline_prog_res.error()));
        return EXIT_FAILURE;
    }
    ShaderProgram outline_prog = *outline_prog_res;

    if (!grid_prog.valid() || !obj_prog.valid() || !outline_prog.valid()) {
        std::println(stderr, "Failed to create shader programs");
        return EXIT_FAILURE;
    }

    GLMesh cube_mesh = create_cube_mesh();
    GLMesh grid_mesh = create_grid_mesh(
        g_render_settings.grid.n_lines_per_side,
        g_render_settings.grid.spacing,
        g_render_settings.grid.axis_alpha,
        g_render_settings.grid.minor_alpha);

    SceneContext scene_context{};
    if (fs::exists(k_scene_path)) {
        auto loaded = load_scene_from_file(k_scene_path);
        if (loaded.has_value()) {
            scene_context = std::move(*loaded);
        }
    }

    if (scene_context.cube_objects.empty()) {
        scene_context.cube_objects.push_back(Object{
            .name = "Cube A",
            .transform = Transform{.position = {2.0f, 1.0f, 0.5f}, .rotation_deg = {0, 0, 0}, .scale = {1, 1, 1}},
            .color = {0.85f, 0.35f, 0.25f},
        });
        scene_context.cube_objects.push_back(Object{
            .name = "Cube B",
            .transform = Transform{.position = {-1.5f, 2.5f, 0.5f}, .rotation_deg = {0, 0, 25}, .scale = {1, 1, 1}},
            .color = {0.25f, 0.55f, 0.90f},
        });
        scene_context.cube_objects.push_back(Object{
            .name = "Cube C",
            .transform = Transform{.position = {-2.5f, -1.5f, 0.75f}, .rotation_deg = {15, 0, 0}, .scale = {1.5f, 1.0f, 1.5f}},
            .color = {0.30f, 0.85f, 0.45f},
        });

        scene_context.camera.pivot = {0, 0, 0};
        scene_context.camera.distance = 10.0f;
        scene_context.selected_index = std::nullopt;
    }

    SceneHotReloader scene_reloader(k_scene_path);
    scene_reloader.init_if_exists();

    bool prev_left{false};
    bool prev_middle{false};
    f64 prev_mx{0.0};
    f64 prev_my{0.0};

    int frame_counter = 0;

    auto last_scene_poll = std::chrono::steady_clock::now();
    constexpr auto scene_poll_interval = std::chrono::milliseconds(250);

    ViewportFBO viewport_fbo{};

    RectInt viewport_fb_rect{};
    bool viewport_fb_rect_valid{false};
    bool viewport_image_hovered{false};

    glm::vec2 viewport_img_pos{0.0f, 0.0f};
    glm::vec2 viewport_img_size{0.0f, 0.0f};

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        { // Hot Reload
            const auto now = std::chrono::steady_clock::now();
            if (now - last_scene_poll >= scene_poll_interval) {
                last_scene_poll = now;

                if (scene_reloader.changed()) {
                    const bool ok = try_hot_reload_scene(scene_context, k_scene_path);
                    if (!ok) {
                        std::println(stderr, "Hot-reload: failed to load {}", k_scene_path);
                    }
                    if (scene_context.selected_index && *scene_context.selected_index >= scene_context.cube_objects.size()) {
                        scene_context.selected_index = std::nullopt;
                    }
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::BeginMainMenuBar()) {
            render_menu_bar(scene_context, scene_reloader);
        }

        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        viewport_fb_rect_valid = false;
        viewport_image_hovered = false;

        { // Viewport window
            ImGuiWindowFlags vp_flags =
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoCollapse;

            ImGui::Begin("Viewport", nullptr, vp_flags);

            const ImVec2 content_pos = ImGui::GetCursorScreenPos();
            const ImVec2 content_size = ImGui::GetContentRegionAvail();
            { // Filling viewport_fb_rect
                viewport_img_pos = glm::vec2{content_pos.x, content_pos.y};
                viewport_img_size = glm::vec2{content_size.x, content_size.y};

                int fbw = 1, fbh = 1;
                glfwGetFramebufferSize(window, &fbw, &fbh);

                int win_w = 1, win_h = 1;
                glfwGetWindowSize(window, &win_w, &win_h);

                const f32 win_w_f = static_cast<f32>(win_w);
                const f32 win_h_f = static_cast<f32>(win_h);
                const f32 fbw_f = static_cast<f32>(fbw);
                const f32 fbh_f = static_cast<f32>(fbh);

                const f32 scale_x = (win_w_f > 0.0f) ? fbw_f / win_w_f : 1.0f;
                const f32 scale_y = (win_h_f > 0.0f) ? fbh_f / win_h_f : 1.0f;
                const f32 left_px = viewport_img_pos.x * scale_x;
                const f32 width_px = viewport_img_size.x * scale_x;
                const f32 height_px = viewport_img_size.y * scale_y;
                const f32 bottom_px = fbh_f - (viewport_img_pos.y + viewport_img_size.y) * scale_y;

                const int vx = static_cast<int>(std::lround(left_px));
                const int vw = static_cast<int>(std::lround(width_px));
                const int vy = static_cast<int>(std::lround(bottom_px));
                const int vh = static_cast<int>(std::lround(height_px));

                viewport_fb_rect.x = std::clamp(vx, 0, std::max(0, fbw - 1));
                viewport_fb_rect.y = std::clamp(vy, 0, std::max(0, fbh - 1));
                viewport_fb_rect.width = std::clamp(vw, 1, fbw - viewport_fb_rect.x);
                viewport_fb_rect.height = std::clamp(vh, 1, fbh - viewport_fb_rect.y);
            }

            const int fbo_w = viewport_fb_rect.width;
            const int fbo_h = viewport_fb_rect.height;

            viewport_fb_rect_valid = (fbo_w > 8 && fbo_h > 8) && viewport_fbo.ensure_size(fbo_w, fbo_h);

            if (viewport_fb_rect_valid) {
                glBindFramebuffer(GL_FRAMEBUFFER, viewport_fbo.fbo);
                glViewport(0, 0, viewport_fbo.width, viewport_fbo.height);

                auto bg = g_render_settings.background_color;
                glClearColor(bg.r(), bg.g(), bg.b(), bg.a());
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

                const f32 aspect = static_cast<f32>(viewport_fbo.width) / static_cast<f32>(viewport_fbo.height);
                const glm::mat4 camera_view_matrix = scene_context.camera.view_matrix();
                const glm::mat4 camera_proj_matrix = scene_context.camera.proj_matrix(aspect);

                { // Grid
                    glDepthMask(GL_FALSE);

                    grid_prog.bind();
                    set_uniform_mat4(grid_prog.id, "uView", camera_view_matrix);
                    set_uniform_mat4(grid_prog.id, "uProj", camera_proj_matrix);
                    set_uniform_float(grid_prog.id, "uFogStart", g_render_settings.grid.fog_start);
                    set_uniform_float(grid_prog.id, "uFogEnd", g_render_settings.grid.fog_end);

                    grid_mesh.vao.bind();
                    glDrawArrays(GL_LINES, 0, grid_mesh.vertex_count);
                    VAO::unbind();

                    glDepthMask(GL_TRUE);
                }

                { // Objects
                    glEnable(GL_DEPTH_TEST);
                    glDepthFunc(GL_LESS);

                    glStencilMask(0x00);
                    glStencilFunc(GL_ALWAYS, 0, 0xFF);

                    obj_prog.bind();
                    set_uniform_mat4(obj_prog.id, "uView", camera_view_matrix);
                    set_uniform_mat4(obj_prog.id, "uProj", camera_proj_matrix);

                    cube_mesh.vao.bind();
                    for (usize i = 0; i < scene_context.cube_objects.size(); ++i) {
                        const Object &o = scene_context.cube_objects[i];
                        const glm::mat4 M = o.transform.model_matrix();

                        set_uniform_mat4(obj_prog.id, "uModel", M);
                        set_uniform_vec3(obj_prog.id, "uColor", o.color);

                        glDrawArrays(GL_TRIANGLES, 0, cube_mesh.vertex_count);
                    }
                    VAO::unbind();
                }

                // Outline via stencil (in FBO)
                if (scene_context.selected_index.has_value() &&
                    *scene_context.selected_index < scene_context.cube_objects.size()) {

                    const Object &sel = scene_context.cube_objects[*scene_context.selected_index];
                    const glm::mat4 M = sel.transform.model_matrix();

                    { // Pass 1: write stencil
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

                        cube_mesh.vao.bind();
                        glDrawArrays(GL_TRIANGLES, 0, cube_mesh.vertex_count);
                        VAO::unbind();

                        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                        glDepthMask(GL_TRUE);
                        glEnable(GL_DEPTH_TEST);
                        glDepthFunc(GL_LESS);
                    }

                    { // Pass 2: draw outline where stencil != 1
                        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
                        glStencilMask(0x00);

                        glDisable(GL_DEPTH_TEST);
                        glDisable(GL_CULL_FACE);

                        outline_prog.bind();

                        const glm::mat4 M_outline = M * glm::scale(glm::mat4(1.0f), glm::vec3(1.04f));

                        set_uniform_mat4(outline_prog.id, "uModel", M_outline);
                        set_uniform_mat4(outline_prog.id, "uView", camera_view_matrix);
                        set_uniform_mat4(outline_prog.id, "uProj", camera_proj_matrix);
                        set_uniform_vec3(outline_prog.id, "uColor", glm::vec3(1.0f, 0.55f, 0.0f));

                        cube_mesh.vao.bind();
                        glDrawArrays(GL_TRIANGLES, 0, cube_mesh.vertex_count);
                        VAO::unbind();

                        glEnable(GL_DEPTH_TEST);
                        glDepthFunc(GL_LESS);

                        glStencilMask(0xFF);
                        glStencilFunc(GL_ALWAYS, 0, 0xFF);
                        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                    }
                }

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                // Present texture in ImGui (flip V)
                ImGui::Image(
                    viewport_fbo.imgui_texture_id(),
                    content_size,
                    ImVec2(0.0f, 1.0f),
                    ImVec2(1.0f, 0.0f));

                viewport_image_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            } else {
                ImGui::TextUnformatted("Viewport too small.");
                viewport_image_hovered = false;
            }

            ImGui::End();
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        // Use ImGui mouse position (stable across multi-viewport)
        const f64 mouse_x = static_cast<f64>(io.MousePos.x);
        const f64 mouse_y = static_cast<f64>(io.MousePos.y);

        const bool left_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool middle_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

        const bool allow_viewport_interaction = viewport_fb_rect_valid && viewport_image_hovered;
        if (allow_viewport_interaction) {
            const f32 wheel = io.MouseWheel;
            if (wheel != 0.0f) {
                constexpr f32 zoom_speed = 0.12f;
                scene_context.camera.distance *= std::exp(-wheel * zoom_speed);
                scene_context.camera.distance = std::clamp(scene_context.camera.distance, 0.75f, 200.0f);
            }

            if (middle_down && prev_middle) {
                const f32 dx = static_cast<f32>(mouse_x - prev_mx);
                const f32 dy = static_cast<f32>(mouse_y - prev_my);

                constexpr f32 sensitivity = 0.0050f;
                scene_context.camera.yaw += -dx * sensitivity;
                scene_context.camera.pitch += dy * sensitivity;

                const f32 lim = glm::radians(89.0f);
                scene_context.camera.pitch = std::clamp(scene_context.camera.pitch, -lim, lim);
            }

            if (left_down && !prev_left) {
                const f32 aspect = static_cast<f32>(viewport_fbo.width) / static_cast<f32>(viewport_fbo.height);
                const glm::mat4 camera_view_matrix = scene_context.camera.view_matrix();
                const glm::mat4 camera_proj_matrix = scene_context.camera.proj_matrix(aspect);

                glm::vec2 mouse_pos = glm::vec2{static_cast<f32>(mouse_x), static_cast<f32>(mouse_y)};

                const Ray ray = ray_from_imgui_rect(
                    mouse_pos,
                    viewport_img_pos,
                    viewport_img_size,
                    camera_view_matrix, camera_proj_matrix);

                std::optional<usize> best_idx{};
                f32 best_t = 1e30f;

                for (usize i = 0; i < scene_context.cube_objects.size(); ++i) {
                    const Object &o = scene_context.cube_objects[i];
                    const glm::mat4 M = o.transform.model_matrix();

                    f32 tW = 0.0f;
                    if (intersect_unit_cube_obb(ray, M, tW)) {
                        if (tW < best_t) {
                            best_t = tW;
                            best_idx = i;
                        }
                    }
                }
                scene_context.selected_index = best_idx;
            }
        }

        prev_left = left_down;
        prev_middle = middle_down;
        prev_mx = mouse_x;
        prev_my = mouse_y;

        render_imgui_windows(scene_context, frame_counter);

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
        ++frame_counter;
    }

    if (!save_scene_to_file(scene_context, k_scene_path)) {
        std::println(stderr, "Warning: failed to save scene to {}", k_scene_path);
    }

    viewport_fbo.destroy();

    glDeleteProgram(grid_prog.id);
    glDeleteProgram(obj_prog.id);
    glDeleteProgram(outline_prog.id);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}