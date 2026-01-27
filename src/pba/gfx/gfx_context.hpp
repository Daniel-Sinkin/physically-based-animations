// pba/gfx/gfx_context.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/core/render_types.hpp"
#include "pba/editor/editor_input.hpp"
#include "pba/gfx/gl_texture.hpp"
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
    struct ShaderPrograms
    {
        ShaderProgram grid{};
        ShaderProgram obj{};
        ShaderProgram obj_tex{};
        ShaderProgram outline{};
        ShaderProgram pivot{};

        void destroy() noexcept
        {
            grid.destroy();
            obj.destroy();
            obj_tex.destroy();
            outline.destroy();
            pivot.destroy();
        }

        [[nodiscard]] bool all_valid() const noexcept
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

    ColorRGBAf background_color{0.255f, 0.255f, 0.255f, 1.0f};

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

        ColorRGBf sleep_active_color{1.0f, 0.0f, 0.0f};
        ColorRGBf sleep_asleep_color{0.0f, 0.0f, 1.0f};

        ColorRGBf ke_low_color{0.0f, 0.0f, 1.0f};
        ColorRGBf ke_high_color{1.0f, 0.0f, 0.0f};
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

    void render_to_viewport();
    void render_to_viewport_grid(const ViewMatrix&, const ProjMatrix&) const;
    void render_to_viewport_objects(const ViewMatrix&, const ProjMatrix&) const;
    void render_to_viewport_pivot(const Position3&, const ViewMatrix&, const ProjMatrix&) const;
    void render_to_viewport_outline(const ViewMatrix&, const ProjMatrix&) const;
    void render_to_viewport_physics_debug(const ViewMatrix&, const ProjMatrix&);

    void viewport_window();

    void step();
    [[nodiscard]] bool setup();

    [[nodiscard]] bool create_programs();
    [[nodiscard]] bool create_meshes();
    [[nodiscard]] bool create_textures();

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
