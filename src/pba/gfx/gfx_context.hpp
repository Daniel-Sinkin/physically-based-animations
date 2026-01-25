// pba/gfx/gfx_context.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/core/render_types.hpp"
#include "pba/editor/editor_input.hpp"
#include "pba/gfx/gl_types.hpp"
#include "pba/gfx/video_recorder.hpp"
#include "pba/gfx/viewport_fbo.hpp"
#include "pba/ui/ui_theme.hpp"

#include <chrono>
#include <filesystem>
#include <glm/vec2.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct GLFWwindow;

namespace ds_pba
{
struct SceneContext;
struct PhysicsContext;
struct EngineContext;
struct Raycast;
struct Camera;

struct GfxContext
{
    GfxContext() = default;
    ~GfxContext();
    GfxContext(const GfxContext&) = delete;
    GfxContext& operator=(const GfxContext&) = delete;
    GfxContext(GfxContext&&) = delete;
    GfxContext& operator=(GfxContext&&) = delete;

    void shutdown();

    EditorInput editor_input{};

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

    bool viewport_valid_warning_shown{false};

    bool imgui_uses_keyboard{};
    bool imgui_uses_mouse{};

    std::unordered_map<std::string, ImFont*> fonts_by_id{};
    ImFont* default_font{};

    glm::vec2 viewport_img_pos{};
    glm::vec2 viewport_img_size{};

    int frame_count{};

    TimePoint run_start{Clock::now()};
    TimePoint last_scene_poll{Clock::now()};
    Duration scene_poll_interval{std::chrono::milliseconds(250)};

    ViewportFBO viewport_fbo{};

    RectInt viewport_fb_rect{};
    bool viewport_fb_rect_valid{false};
    bool viewport_image_hovered{false};

    SceneContext* scene_context{};
    // This is only intended for ImGUI don't access values for the GfxContext through it
    EngineContext* engine_context{};

    bool is_active_{true};

    bool pivot_active{true};

    VideoRecorder recorder{};

    std::vector<u8> capture_rgba{};
    int capture_fps{k_video_recorder_fps};
    std::filesystem::path capture_output_dir{"renders"};
    usize capture_take_index{0zu};

    void start_recording();
    void stop_recording();
    void toggle_recording();
    [[nodiscard]] bool capture_viewport_rgba8(std::vector<u8>& out) const;

    GLMesh cube_mesh{};
    GLMesh sphere_mesh{};
    GLMesh grid_mesh{};
    GLMesh marble_bust_mesh{};
    GLMesh pyramid_mesh{};
    GLMesh cylinder_mesh{};

    GridSettings grid{};

    struct EditorState
    {
        enum class GrabConstraint : u8
        {
            None,
            X,
            Y,
            Z
        };

        struct Grab
        {
            bool active{false};
            f64 start_mouse_x{0.0};
            f64 start_mouse_y{0.0};
            GrabConstraint constraint{GrabConstraint::None};
            std::vector<std::pair<ObjectId, Position3>> start_positions{};
        };

        Grab grab{};
    };
    EditorState editor{};

    void begin_grab(const EditorInput& input);
    void update_grab(const EditorInput& input);
    void cancel_grab();
    void confirm_grab();
    void set_grab_constraint(EditorState::GrabConstraint c);

    void render_to_viewport() const;
    void render_to_viewport_grid(const ViewMatrix&, const ProjMatrix&) const;
    void render_to_viewport_objects(const ViewMatrix&, const ProjMatrix&) const;
    void render_to_viewport_pivot(const Position3&, const ViewMatrix&, const ProjMatrix&) const;
    void render_to_viewport_outline(const ViewMatrix&, const ProjMatrix&) const;

    void viewport_window();

    void step();
    bool setup();

    bool create_programs();
    bool create_meshes();

    void hover_interaction(const EditorInput& input) const;
    void hover_interaction_holding_middle(const EditorInput& input, Camera& cam) const;
    void hover_interaction_selection(const EditorInput& input, const Raycast& rc) const;

    void set_object_position(ObjectId id, const Position3& p) const;
    [[nodiscard]] std::optional<Position3> get_object_position(ObjectId id) const;

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
