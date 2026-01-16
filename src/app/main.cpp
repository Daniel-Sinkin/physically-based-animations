// pba/main.cpp
#include "pba/glfw_setup.hpp"
#include "pba/mesh.hpp"
#include "pba/camera.hpp"
#include "pba/interaction.hpp"
#include "pba/render_settings.hpp"
#include "pba/ui.hpp"
#include "pba/gl.hpp"
#include "pba/mesh.hpp"
#include "pba/render_settings.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <print>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"


int main() {
    using namespace ds_pba;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    auto window_res = setup_glfw();
    if (!window_res.has_value()) {
        std::println(stderr, "Failed to setup glfw with error code: {}", static_cast<int>(window_res.error()));
        return EXIT_FAILURE;
    }
    GLFWwindow *window = *window_res;

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
    // GLMesh grid_mesh = create_grid_mesh(g_render_settings.grid);
    GLMesh grid_mesh = ds_pba::create_grid_mesh(
        g_render_settings.grid.n_lines_per_side,
        g_render_settings.grid.spacing,
        g_render_settings.grid.axis_alpha,
        g_render_settings.grid.minor_alpha);

    std::vector<Object> cube_objects;
    cube_objects.push_back(Object{
        .name = "Cube A",
        .transform = Transform{.position = {2.0f, 1.0f, 0.5f}, .rotation_deg = {0, 0, 0}, .scale = {1, 1, 1}},
        .color = {0.85f, 0.35f, 0.25f},
    });
    cube_objects.push_back(Object{
        .name = "Cube B",
        .transform = Transform{.position = {-1.5f, 2.5f, 0.5f}, .rotation_deg = {0, 0, 25}, .scale = {1, 1, 1}},
        .color = {0.25f, 0.55f, 0.90f},
    });
    cube_objects.push_back(Object{
        .name = "Cube C",
        .transform = Transform{.position = {-2.5f, -1.5f, 0.75f}, .rotation_deg = {15, 0, 0}, .scale = {1.5f, 1.0f, 1.5f}},
        .color = {0.30f, 0.85f, 0.45f},
    });

    std::optional<usize> selected_index = std::nullopt;

    Camera camera{};
    camera.pivot = {0, 0, 0};
    camera.distance = 10.0f;

    auto framebuffer_callback = [](GLFWwindow *, int width, int height) -> void {
        glViewport(0, 0, width, height);
    };
    glfwSetFramebufferSizeCallback(window, framebuffer_callback);

    {
        int fbw = 0, fbh = 0;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
    }

    bool prev_left = false;
    bool prev_middle = false;
    f64 prev_mx = 0.0;
    f64 prev_my = 0.0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiIO &io = ImGui::GetIO();
        const bool imgui_wants_mouse = io.WantCaptureMouse;

        { // Handle Inputs
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }

        f64 mouse_x = 0.0;
        f64 mouse_y = 0.0;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);

        const bool left_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool middle_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

        if (!imgui_wants_mouse) {
            const f32 wheel = io.MouseWheel;
            if (wheel != 0.0f) {
                constexpr f32 zoom_speed = 0.12f;
                camera.distance *= std::exp(-wheel * zoom_speed);
                camera.distance = std::clamp(camera.distance, 0.75f, 200.0f);
            }
        }

        if (middle_down && !imgui_wants_mouse) {
            if (prev_middle) {
                const f32 dx = static_cast<f32>(mouse_x - prev_mx);
                const f32 dy = static_cast<f32>(mouse_y - prev_my);

                constexpr f32 sensitivity = 0.0050f;
                camera.yaw += -dx * sensitivity;
                camera.pitch += dy * sensitivity;

                const f32 lim = glm::radians(89.0f);
                camera.pitch = std::clamp(camera.pitch, -lim, lim);
            }
        }

        if (left_down && !prev_left && !imgui_wants_mouse) {
            int fbw = 1, fbh = 1;
            glfwGetFramebufferSize(window, &fbw, &fbh);
            const f32 aspect = (fbh > 0) ? (static_cast<f32>(fbw) / static_cast<f32>(fbh)) : 1.0f;

            const glm::mat4 camera_view_matrix = camera.view_matrix();
            const glm::mat4 camera_proj_matrix = camera.proj_matrix(aspect);

            const Ray ray = ray_from_mouse(window, mouse_x, mouse_y, camera_view_matrix, camera_proj_matrix);

            std::optional<usize> best_idx{};
            f32 best_t = 1e30f;

            for (usize i = 0; i < cube_objects.size(); ++i) {
                const Object &o = cube_objects[i];
                const glm::mat4 M = o.transform.model_matrix();

                f32 tW = 0.0f;
                if (intersect_unit_cube_obb(ray, M, tW)) {
                    if (tW < best_t) {
                        best_t = tW;
                        best_idx = i;
                    }
                }
            }
            selected_index = best_idx;
        }

        prev_left = left_down;
        prev_middle = middle_down;
        prev_mx = mouse_x;
        prev_my = mouse_y;

        render_imgui_windows(camera, cube_objects, selected_index);
        ImGui::Render();

        int frame_buffer_width = 1, frame_buffer_height = 1;
        glfwGetFramebufferSize(window, &frame_buffer_width, &frame_buffer_height);
        const f32 aspect =
            (frame_buffer_height > 0) ? (static_cast<f32>(frame_buffer_width) / static_cast<f32>(frame_buffer_height)) : 1.0f;

        const glm::mat4 V = camera.view_matrix();
        const glm::mat4 P = camera.proj_matrix(aspect);

        auto bg = g_render_settings.background_color;
        glClearColor(bg.r(), bg.g(), bg.b(), bg.a());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        { // Grid
            glDepthMask(GL_FALSE);

            grid_prog.bind();
            set_uniform_mat4(grid_prog.id, "uView", V);
            set_uniform_mat4(grid_prog.id, "uProj", P);
            set_uniform_float(grid_prog.id, "uFogStart", g_render_settings.grid.fog_start);
            set_uniform_float(grid_prog.id, "uFogEnd", g_render_settings.grid.fog_end);
            set_uniform_float(grid_prog.id, "uMinorAlpha", g_render_settings.grid.minor_alpha);
            set_uniform_float(grid_prog.id, "uAxisAlpha", g_render_settings.grid.axis_alpha);

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
            set_uniform_mat4(obj_prog.id, "uView", V);
            set_uniform_mat4(obj_prog.id, "uProj", P);
            set_uniform_vec3(obj_prog.id, "uCameraPos", camera.position());
            set_uniform_float(obj_prog.id, "uTime", static_cast<float>(glfwGetTime()));

            cube_mesh.vao.bind();
            for (usize i = 0; i < cube_objects.size(); ++i) {
                const Object &o = cube_objects[i];
                const glm::mat4 M = o.transform.model_matrix();

                set_uniform_mat4(obj_prog.id, "uModel", M);
                set_uniform_vec3(obj_prog.id, "uColor", o.color);

                glDrawArrays(GL_TRIANGLES, 0, cube_mesh.vertex_count);
            }
            VAO::unbind();
        }

        // Outline Stencil
        if (selected_index.has_value()) {
            { // First Pass (write stencil)
                const Object &o = cube_objects[*selected_index];
                const glm::mat4 M = o.transform.model_matrix();

                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                glDepthMask(GL_FALSE);
                glDisable(GL_DEPTH_TEST);

                glStencilMask(0xFF);
                glStencilFunc(GL_ALWAYS, 1, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

                obj_prog.bind();
                set_uniform_mat4(obj_prog.id, "uView", V);
                set_uniform_mat4(obj_prog.id, "uProj", P);
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
                const Object &o = cube_objects[*selected_index];

                glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
                glStencilMask(0x00);

                glDisable(GL_DEPTH_TEST);
                glDisable(GL_CULL_FACE);

                outline_prog.bind();

                const glm::mat4 M = o.transform.model_matrix();
                const glm::mat4 M_outline = M * glm::scale(glm::mat4(1.0f), glm::vec3(1.04f));

                set_uniform_mat4(outline_prog.id, "uModel", M_outline);
                set_uniform_mat4(outline_prog.id, "uView", V);
                set_uniform_mat4(outline_prog.id, "uProj", P);
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

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
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