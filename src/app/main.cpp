// pba/main.cpp
#include "glm/fwd.hpp"
#include "pba/camera.hpp"
#include "pba/render_context.hpp"
#include "pba/scene_context.hpp"
#include "pba/scene_types.hpp"
#include "pba/types.hpp" // IWYU pragma: keep
#include "pba/viewport_fbo.hpp"

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

    if (scene_context.cube_objects.empty()) {
        scene_context.cube_objects.push_back(Object{
            .type = ObjectType::Cube,
            .name = "Cube A",
            .transform = Transform{.position = {2.0f, 1.0f, 0.5f}, .rotation_deg = {0, 0, 0}, .scale = {1, 1, 1}},
            .color = {0.85f, 0.35f, 0.25f},
        });
        scene_context.cube_objects.push_back(Object{
            .type = ObjectType::Cube,
            .name = "Cube B",
            .transform = Transform{.position = {-1.5f, 2.5f, 0.5f}, .rotation_deg = {0, 0, 25}, .scale = {1, 1, 1}},
            .color = {0.25f, 0.55f, 0.90f},
        });
        scene_context.cube_objects.push_back(Object{
            .type = ObjectType::Cube,
            .name = "Cube C",
            .transform = Transform{.position = {-2.5f, -1.5f, 0.75f}, .rotation_deg = {15, 0, 0}, .scale = {1.5f, 1.0f, 1.5f}},
            .color = {0.30f, 0.85f, 0.45f},
        });
        scene_context.cube_objects.push_back(Object{
            .type = ObjectType::Cube,
            .name = "Cube D",
            .transform = Transform{.position = {-0.5f, -1.5f, 0.75f}, .rotation_deg = {15, 0, 0}, .scale = {1.5f, 1.0f, 1.5f}},
            .color = {0.30f, 0.85f, 0.45f},
        });
        scene_context.cube_objects.push_back(Object{
            .type = ObjectType::Cube,
            .name = "Cube E",
            .transform = Transform{.position = {-4.5f, -1.5f, 0.75f}, .rotation_deg = {15, 0, 0}, .scale = {1.5f, 1.0f, 1.5f}},
            .color = {1.0f, 0.85f, 0.45f},
        });
        scene_context.cube_objects.push_back(Object{
            .type = ObjectType::Cube,
            .name = "Cube E",
            .transform = Transform{.position = {-10.5f, -1.5f, 0.75f}, .rotation_deg = {15, 0, 0}, .scale = {1.5f, 1.0f, 1.5f}},
            .color = {1.0f, 0.85f, 0.6f},
        });
        scene_context.sphere_objects.push_back(Object{
            .type = ObjectType::Sphere,
            .name = "Sphere A",
            .transform = Transform{},
            .color = {1.0f, 0.6f, 0.3f},
        });
        scene_context.camera.pivot = {0.0f, 0.0f, 0.0f};
        scene_context.camera.distance = 10.0f;
        scene_context.selected_index = std::nullopt;
        scene_context.selected_type = std::nullopt;
    }

    scene_context.reloader = std::make_unique<SceneHotReloader>(SceneHotReloader(k_scene_path));
    scene_context.reloader->init_if_exists();
    return scene_context;
}

int main() {
    using namespace ds_pba;

    std::println("Hello, World!");
    std::println("Hello, World2!");
    std::println("Hello, World2!");
    std::println("Hello, World3!");

    RenderContext render_context{};
    if (!render_context.setup()) {
        return EXIT_FAILURE;
    }
    render_context.scene_context = std::make_unique<SceneContext>(setup_scene());

    render_context.run();

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
