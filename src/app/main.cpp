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

ds_pba::SceneContext setup_scene() {
    using namespace ds_pba;
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
    return scene_context;
}


int main() {
    using namespace ds_pba;

    RenderContext render_context{};
    if(!render_context.setup()) {
        return EXIT_FAILURE;
    }
    render_context.scene_context = std::make_unique<SceneContext>(setup_scene());

    render_context.run();

    if (!save_scene_to_file(*render_context.scene_context, k_scene_path)) {
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