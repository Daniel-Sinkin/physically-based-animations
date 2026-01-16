// pba/main.cpp
#include "pba/camera.hpp"
#include "pba/gl.hpp"
#include "pba/glfw_setup.hpp"
#include "pba/interaction.hpp"
#include "pba/mesh.hpp"
#include "pba/render_settings.hpp"
#include "pba/scene_context.hpp"
#include "pba/types.hpp"
#include "pba/ui.hpp"

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

namespace ds_pba {

[[nodiscard]] static RectInt compute_main_work_render_rect(GLFWwindow *window) {
    ImGuiViewport *vp = ImGui::GetMainViewport();
    const ImVec2 wp = vp->WorkPos;  // top-left in screen coords
    const ImVec2 ws = vp->WorkSize; // size in screen coords

    int fbw = 1, fbh = 1;
    glfwGetFramebufferSize(window, &fbw, &fbh);

    int win_w = 1, win_h = 1;
    glfwGetWindowSize(window, &win_w, &win_h);

    const float sx = (win_w > 0) ? (static_cast<float>(fbw) / static_cast<float>(win_w)) : 1.0f;
    const float sy = (win_h > 0) ? (static_cast<float>(fbh) / static_cast<float>(win_h)) : 1.0f;

    // X is straightforward: screen-space left -> framebuffer-space left.
    const int vx = static_cast<int>(std::lround(wp.x * sx));
    const int vw = static_cast<int>(std::lround(ws.x * sx));

    // Y: ImGui is top-left; OpenGL viewport is bottom-left.
    // Compute (in screen space) how far WorkPos is from the viewport top, then flip to bottom.
    const float vp_top = vp->Pos.y;
    const float vp_h = vp->Size.y;

    const float work_top = wp.y;
    const float work_h = ws.y;

    const float work_y_from_top = (work_top - vp_top);
    const int vy = static_cast<int>(std::lround((vp_h - (work_y_from_top + work_h)) * sy));
    const int vh = static_cast<int>(std::lround(work_h * sy));

    RectInt r{};
    r.x = std::clamp(vx, 0, std::max(0, fbw - 1));
    r.y = std::clamp(vy, 0, std::max(0, fbh - 1));
    r.width = std::clamp(vw, 1, fbw - r.x);
    r.height = std::clamp(vh, 1, fbh - r.y);
    return r;
}

} // namespace ds_pba

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

    // Scene init / load
    SceneContext scene_context{};
    if (fs::exists(k_scene_path)) {
        auto loaded = load_scene_from_file(k_scene_path);
        if (loaded) {
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

    bool prev_left = false;
    bool prev_middle = false;
    f64 prev_mx = 0.0;
    f64 prev_my = 0.0;

    int frame_counter = 0;

    auto last_scene_poll = std::chrono::steady_clock::now();
    constexpr auto scene_poll_interval = std::chrono::milliseconds(250);

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

        // Menu bar (must be before computing WorkPos/WorkSize-based render rect)
        if (ImGui::BeginMainMenuBar()) {
            render_menu_bar(scene_context, scene_reloader);
        }

        // Docking (passthrough central node so it doesn't paint an opaque background over the 3D view)
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        // Compute render rect now that menu bar/docking are set up for the frame
        const RectInt rr = compute_main_work_render_rect(window);

        // Basic input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        f64 mouse_x = 0.0;
        f64 mouse_y = 0.0;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);

        const bool left_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool middle_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

        const auto mouse_in_view = [&]() -> bool {
            int fbw = 1, fbh = 1;
            glfwGetFramebufferSize(window, &fbw, &fbh);

            int win_w = 1, win_h = 1;
            glfwGetWindowSize(window, &win_w, &win_h);

            const float sx = (win_w > 0) ? (static_cast<f32>(fbw) / static_cast<f32>(win_w)) : 1.0f;
            const float sy = (win_h > 0) ? (static_cast<f32>(fbh) / static_cast<f32>(win_h)) : 1.0f;

            const float mx_fb = static_cast<f32>(mouse_x) * sx;
            const float my_fb = static_cast<f32>(mouse_y) * sy;
            const float my_gl = static_cast<f32>(fbh) - my_fb;

            return (mx_fb >= static_cast<f32>(rr.x) && mx_fb < static_cast<f32>(rr.x + rr.width) &&
                    my_gl >= static_cast<f32>(rr.y) && my_gl < static_cast<f32>(rr.y + rr.height));
        }();

        const bool imgui_wants_mouse = io.WantCaptureMouse;

        const f32 aspect = (rr.height > 0) ? (static_cast<f32>(rr.width) / static_cast<f32>(rr.height)) : 1.0f;
        const glm::mat4 camera_view_matrix = scene_context.camera.view_matrix();
        const glm::mat4 camera_proj_matrix = scene_context.camera.proj_matrix(aspect);

        // Camera controls only when the mouse is over the 3D view and ImGui doesn't want it
        if (!imgui_wants_mouse && mouse_in_view) {
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
                const Ray ray = ray_from_mouse_in_rect(
                    window, mouse_x, mouse_y,
                    rr.x, rr.y, rr.width, rr.height,
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

        glEnable(GL_SCISSOR_TEST);
        glViewport(rr.x, rr.y, rr.width, rr.height);
        glScissor(rr.x, rr.y, rr.width, rr.height);

        auto bg = g_render_settings.background_color;
        glClearColor(bg.r(), bg.g(), bg.b(), bg.a());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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

        // Outline stencil
        if (scene_context.selected_index.has_value()) {
            { // First pass (write stencil)
                const Object &o = scene_context.cube_objects[*scene_context.selected_index];
                const glm::mat4 M = o.transform.model_matrix();

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

            { // Second pass (draw outline where stencil != 1)
                const Object &o = scene_context.cube_objects[*scene_context.selected_index];

                glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
                glStencilMask(0x00);

                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);

                outline_prog.bind();

                const glm::mat4 M = o.transform.model_matrix();
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

        glDisable(GL_SCISSOR_TEST);

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

    glDeleteProgram(grid_prog.id);
    glDeleteProgram(obj_prog.id);
    glDeleteProgram(outline_prog.id);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}