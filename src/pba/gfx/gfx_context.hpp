// pba/gfx/gfx_context.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/geometry.hpp"
#include "pba/core/math_types.hpp"
#include "pba/core/render_types.hpp"
#include "pba/editor/editor_input.hpp"
#include "pba/gfx/gl_texture.hpp"
#include "pba/gfx/gl_types.hpp"
#include "pba/gfx/shader_program.hpp"
#include "pba/gfx/video_recorder.hpp"
#include "pba/gfx/viewport_fbo.hpp"
#include "pba/scene/entity_id.hpp"
#include "pba/ui/ui_theme.hpp"

#include <chrono>
#include <filesystem>
#include <glm/vec2.hpp>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

struct GLFWwindow;

namespace ds_pba
{
struct SceneContext;
struct PhysicsContext;
struct EngineContext;
struct Raycast;
struct Camera;
struct Entity;
struct RigidBody;

struct GfxContext
{
    GfxContext() = default;
    ~GfxContext();
    GfxContext(const GfxContext&) = delete;
    GfxContext& operator=(const GfxContext&) = delete;
    GfxContext(GfxContext&&) = delete;
    GfxContext& operator=(GfxContext&&) = delete;

    void shutdown();

    GLFWwindow* window{};
    struct ShaderPrograms
    {
        ShaderProgram grid{};
        ShaderProgram obj{};
        ShaderProgram obj_tex{};
        ShaderProgram outline{};
        ShaderProgram pivot{};

        auto destroy() noexcept -> void
        {
            grid.destroy();
            obj.destroy();
            obj_tex.destroy();
            outline.destroy();
            pivot.destroy();
        }

        [[nodiscard]] auto all_valid() const noexcept -> bool
        {
            return grid.valid() && obj.valid() && obj_tex.valid() && outline.valid()
                   && pivot.valid();
        }
    };
    ShaderPrograms shader_programs{};

    bool initialised_glfw{false};
    bool window_created{false};
    bool loaded_glad{false};
    bool initialised_imgui{false};

    Color3 background_color{0.102f, 0.106f, 0.149f};

    UiThemePack theme_pack{};
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

    EngineContext* engine_context{};

    bool is_active_{true};

    bool pivot_active{true};

    VideoRecorder recorder{};

    std::vector<u8> capture_rgba{};
    int capture_fps{k_video_recorder_fps};
    std::filesystem::path capture_output_dir{"renders"};
    usize capture_take_index{0zu};

    auto start_recording() -> void;
    auto stop_recording() -> void;
    auto toggle_recording() -> void;
    [[nodiscard]] auto capture_viewport_rgba8(std::vector<u8>& out) const -> bool;

    struct Textures
    {
        GLTexture2D clean_asphalt_diffuse{};
        GLTexture2D clean_asphalt_normal{};

        GLTexture2D marble_bust_diffuse{};
        GLTexture2D marble_bust_normal{};

        void destroy() noexcept
        {
            clean_asphalt_diffuse.destroy();
            clean_asphalt_normal.destroy();

            marble_bust_diffuse.destroy();
            marble_bust_normal.destroy();
        }
    };
    Textures textures{};

    struct Meshes
    {
        GLMesh cube{};
        GLMesh sphere{};
        GLMesh grid{};
        GLMesh marble_bust{};
        GLMesh pyramid{};
        GLMesh cylinder{};
        GLMesh debug_line{};

        void destroy() noexcept
        {
            cube.destroy();
            sphere.destroy();
            grid.destroy();
            marble_bust.destroy();
            pyramid.destroy();
            cylinder.destroy();
            debug_line.destroy();
        }
    };
    Meshes meshes{};

    GridSettings grid{};

    struct PhysicsDebugSettings
    {
        bool enabled{true};

        enum class ColorMode : u8
        {
            Diffuse = 0,
            SleepState,
            KineticEnergy,
        };

        ColorMode color_mode{ColorMode::Diffuse};

        Color3 sleep_active_color{1.0f, 0.0f, 0.0f};
        Color3 sleep_asleep_color{0.0f, 0.0f, 1.0f};

        Color3 ke_low_color{0.0f, 0.0f, 1.0f};
        Color3 ke_high_color{1.0f, 0.0f, 0.0f};
        f32 ke_max{1.0f};
        bool ke_include_angular{true};

        bool show_selected_axes{true};
        bool show_selected_velocity{true};
        bool show_selected_angular_velocity{true};

        bool show_contacts{true};
        bool show_contact_normals{false};

        bool depth_test{false};

        f32 axis_scale{1.25f};
        f32 vel_scale{0.10f};
        f32 ang_vel_scale{0.10f};
        f32 contact_marker_size{0.05f};
        f32 contact_normal_scale{0.25f};
    };

    PhysicsDebugSettings phys_debug{};

    struct DebugLineV_PColor
    {
        f32 px, py, pz;
        f32 r, g, b, a;
    };
    static_assert(sizeof(DebugLineV_PColor) == 7 * sizeof(f32));
    static_assert(offsetof(DebugLineV_PColor, r) == 3 * sizeof(f32));

    bool debug_line_created{false};
    std::vector<DebugLineV_PColor> debug_line_vertices{};

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
            std::vector<std::pair<EntityId, Pos3>> start_positions{};
        };

        Grab grab{};
    };
    EditorState editor{};

    auto begin_grab(const EditorInput& input) -> void;
    auto update_grab(const EditorInput& input) -> void;
    auto cancel_grab() -> void;
    auto confirm_grab() -> void;
    auto set_grab_constraint(EditorState::GrabConstraint c) -> void;

    auto render_to_viewport() -> void;
    auto render_to_viewport_grid(const ViewMatrix&, const ProjMatrix&) const -> void;
    auto render_to_viewport_objects(const ViewMatrix&, const ProjMatrix&) const -> void;
    auto render_to_viewport_pivot(const Pos3&, const ViewMatrix&, const ProjMatrix&) const -> void;
    auto render_to_viewport_outline(const ViewMatrix&, const ProjMatrix&) const -> void;
    auto render_to_viewport_physics_debug(const ViewMatrix&, const ProjMatrix&) -> void;

    auto viewport_window() -> void;

    auto step(EditorInput&) -> void;
    [[nodiscard]] auto setup() -> bool;

    [[nodiscard]] auto create_programs() -> bool;
    [[nodiscard]] auto create_meshes() -> bool;
    [[nodiscard]] auto create_textures() -> bool;

    auto hover_interaction(EditorInput& input) const -> void;
    auto hover_interaction_holding_middle(EditorInput& input, Camera& cam) const -> void;
    auto hover_interaction_selection(EditorInput& input, const Raycast& rc) const -> void;

  private:
    // Physics debug rendering helpers (split from render_to_viewport_physics_debug)
    [[nodiscard]] bool should_render_physics_debug_() const noexcept;

    auto push_debug_line_(const Pos3& a, const Pos3& b, f32 r, f32 g, f32 bl, f32 al) noexcept
        -> void;
    auto append_contact_debug_lines_() -> void;
    auto append_selected_entity_debug_lines_() -> void;

    [[nodiscard]] auto
    selected_entity_frame_(const Entity& entity, const RigidBody* rigid_body) const noexcept
        -> std::tuple<Pos3, Quaternion, f32>;

    auto append_selected_velocity_debug_lines_(const RigidBody& rigid_body, const Pos3& com)
        -> void;
    auto append_selected_angular_velocity_debug_lines_(const RigidBody& rigid_body, const Pos3& com)
        -> void;

    auto ensure_debug_line_mesh_created_() -> void;
    auto upload_debug_lines_to_gpu_() -> void;

    auto configure_physics_debug_gl_state_() const noexcept -> void;

    auto draw_debug_lines_(
        const ViewMatrix& camera_view_matrix, const ProjMatrix& camera_proj_matrix
    ) const -> void;
};
}  // namespace ds_pba
