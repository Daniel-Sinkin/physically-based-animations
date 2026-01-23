// pba/render_context.hpp
#pragma once

#include "pba/core_types.hpp"
#include "pba/gl_types.hpp"
#include "pba/ui_theme.hpp"
#include "pba/viewport_fbo.hpp"

#include <chrono>
#include <glm/vec2.hpp>

struct GLFWwindow;

namespace ds_pba
{
struct SceneContext;
struct PhysicsContext;
struct EngineContext;

struct GridSettings
{
    int n_lines_per_side{30};
    f32 spacing{1.0f};
    f32 fog_start{12.0f};
    f32 fog_end{30.0f};
    f32 minor_alpha{0.35f};
    f32 axis_alpha{0.95f};
};

constexpr f32 zoom_speed{0.12f};
constexpr f32 sensitivity{0.0050f};
constexpr f32 pan_sensitivity{1.00f};

struct SceneContext;

struct RenderContext
{
    ~RenderContext();

    void shutdown();

    GLFWwindow* window{};
    ShaderProgram grid_prog{};
    ShaderProgram obj_prog{};
    ShaderProgram outline_prog{};
    ShaderProgram pivot_prog{};

    bool initialised_glfw{false};
    bool window_created{false};
    bool loaded_glad{false};
    bool initialised_imgui{false};

    ColorRGBAf background_color{0.255f, 0.255f, 0.255f, 1.0f};

    ui_theme::UiThemePack theme_pack{};
    usize theme_index{0zu};
    bool theme_loaded{false};

    std::unordered_map<std::string, ImFont*> fonts_by_id{};
    ImFont* default_font{};

    glm::vec2 viewport_img_pos{};
    glm::vec2 viewport_img_size{};

    bool prev_left{false};
    bool prev_middle{false};
    bool prev_right{false};
    bool prev_f1{false};
    f64 prev_mx{0.0};
    f64 prev_my{0.0};

    int frame_count{0};

    TimePoint run_start = Clock::now();
    TimePoint last_scene_poll = Clock::now();
    Duration scene_poll_interval = std::chrono::milliseconds(250);

    ViewportFBO viewport_fbo{};

    RectInt viewport_fb_rect{};
    bool viewport_fb_rect_valid{false};
    bool viewport_image_hovered{false};

    SceneContext* scene_context{};
    EngineContext* engine_context{};  // Only for ImGUI

    bool is_active_{true};

    bool pivot_active{true};

    GLMesh cube_mesh{};
    GLMesh sphere_mesh{};
    GLMesh grid_mesh{};
    GLMesh marble_bust_mesh{};
    GLMesh pyramid_mesh{};
    GLMesh cylinder_mesh{};

    GridSettings grid{};

    void render_to_viewport();
    void viewport_window();

    void step();
    bool setup();

    bool create_programs();
    bool create_meshes();

    bool is_active() const;
    void deactivate() noexcept
    {
        is_active_ = false;
    }
    void request_close() noexcept;
    void activate() noexcept
    {
        is_active_ = true;
    }
};
}  // namespace ds_pba
