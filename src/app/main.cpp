// pba/main.cpp
#include "glm/fwd.hpp"
#include "pba/camera.hpp"
#include "pba/gl.hpp"
#include "pba/glfw_setup.hpp"
#include "pba/interaction.hpp"
#include "pba/mesh.hpp"
#include "pba/scene_context.hpp"
#include "pba/types.hpp" // IWYU pragma: keep
#include "pba/ui.hpp"
#include "pba/viewport_fbo.hpp"
#include "pba/render_context.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <print>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

int main() {
    using namespace ds_pba;

    RenderContext render_context{};

    auto window_res = setup_glfw();
    if (!window_res) {
        std::println(stderr, "Failed to setup glfw with error code: {}", static_cast<int>(window_res.error()));
        return EXIT_FAILURE;
    }
    render_context.window = *window_res;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    apply_blender_style();

    const char *glsl_version = "#version 330";
    if (!ImGui_ImplGlfw_InitForOpenGL(render_context.window, true)) {
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
    render_context.grid_prog = *grid_prog_res;

    auto obj_prog_res = create_program_from_file("object");
    if (!obj_prog_res) {
        std::println(stderr, "Failed to load 'object' shaders, got error code: {}", static_cast<int>(obj_prog_res.error()));
        return EXIT_FAILURE;
    }
    render_context.obj_prog = *obj_prog_res;

    auto outline_prog_res = create_program_from_file("outline");
    if (!outline_prog_res) {
        std::println(stderr, "Failed to load 'outline' shaders, got error code: {}", static_cast<int>(outline_prog_res.error()));
        return EXIT_FAILURE;
    }
    render_context.outline_prog = *outline_prog_res;

    if (!render_context.grid_prog.valid() || !render_context.obj_prog.valid() || !render_context.outline_prog.valid()) {
        std::println(stderr, "Failed to create shader programs");
        return EXIT_FAILURE;
    }

    render_context.cube_mesh = create_cube_mesh();
    render_context.grid_mesh = create_grid_mesh(
        render_context.grid.n_lines_per_side,
        render_context.grid.spacing,
        render_context.grid.axis_alpha,
        render_context.grid.minor_alpha);

    SceneContext scene_context;
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

    scene_context.reloader = std::make_unique<SceneHotReloader>(SceneHotReloader(k_scene_path));
    scene_context.reloader->init_if_exists();

    render_context.scene_context = std::make_unique<SceneContext>(std::move(scene_context));

    render_context.last_scene_poll = std::chrono::steady_clock::now();

    render_context.viewport_img_pos = {0.0f, 0.0f};
    render_context.viewport_img_size = {0.0f, 0.0f};

    render_context.run();

    if (!save_scene_to_file(scene_context, k_scene_path)) {
        std::println(stderr, "Warning: failed to save scene to {}", k_scene_path);
    }

    render_context.viewport_fbo.destroy();

    glDeleteProgram(render_context.grid_prog.id);
    glDeleteProgram(render_context.obj_prog.id);
    glDeleteProgram(render_context.outline_prog.id);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(render_context.window);
    glfwTerminate();
}