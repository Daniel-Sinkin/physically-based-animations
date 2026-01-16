// pba/scene_context->hpp
#pragma once

#include "pba/scene_context.hpp"
#include "pba/types.hpp" // IWYU pragma: keep
#include "pba/viewport_fbo.hpp"

#include <chrono>
#include <memory>

struct GLFWwindow; 

namespace ds_pba {
struct GridSettings {
    int n_lines_per_side = 30;
    f32 spacing = 1.0f;
    f32 fog_start = 12.0f;
    f32 fog_end = 30.0f;
    f32 minor_alpha = 0.35f;
    f32 axis_alpha = 0.95f;
};

struct RenderContext {
    GLFWwindow *window{};
    ShaderProgram grid_prog{};
    ShaderProgram obj_prog{};
    ShaderProgram outline_prog{};

    ColorRGBAf background_color{0.255f, 0.255f, 0.255f, 1.0f};

    glm::vec2 viewport_img_pos{};
    glm::vec2 viewport_img_size{};

    bool prev_left{false};
    bool prev_middle{false};
    f64 prev_mx{0.0};
    f64 prev_my{0.0};

    int frame_counter = 0;

    TimePoint last_scene_poll = std::chrono::steady_clock::now();
    Duration scene_poll_interval = std::chrono::milliseconds(250);

    ViewportFBO viewport_fbo{};

    RectInt viewport_fb_rect{};
    bool viewport_fb_rect_valid{false};
    bool viewport_image_hovered{false};

    std::unique_ptr<SceneContext> scene_context{};

    GLMesh cube_mesh{};
    GLMesh grid_mesh{};

    GridSettings grid{};

    void run();
};
} // namespace ds_pba