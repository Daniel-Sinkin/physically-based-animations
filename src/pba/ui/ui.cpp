// pba/ui/ui.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/ui/ui.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/engine/engine_context.hpp"
#include "pba/gfx/gfx_context.hpp"
#include "pba/physics/physics_context.hpp"
#include "pba/simulation/scenes.hpp"

//
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>

namespace ds_pba
{
namespace
{
struct RollingFrameStats
{
    static constexpr usize k_window_samples{512zu};

    struct Snapshot
    {
        usize sample_count{};
        f64 window_seconds{};
        f64 avg_frame_ms{};
        f64 fps{};
        f64 p50_frame_ms{};
        f64 p95_frame_ms{};
        f64 p99_frame_ms{};
        f64 max_frame_ms{};
        bool valid{false};
    };

    std::array<f32, k_window_samples> samples_ms{};
    usize next_write{0zu};
    usize sample_count{0zu};
    TimePoint last_frame_time{};
    bool has_last_frame_time{false};

    auto tick(TimePoint now) noexcept -> void
    {
        if (!has_last_frame_time)
        {
            last_frame_time = now;
            has_last_frame_time = true;
            return;
        }

        const auto dt_ms = std::chrono::duration<f64, std::milli>(now - last_frame_time).count();
        last_frame_time = now;

        if (!std::isfinite(dt_ms) || dt_ms <= 0.0)
        {
            return;
        }

        samples_ms[next_write] = static_cast<f32>(dt_ms);
        next_write = (next_write + 1zu) % k_window_samples;
        sample_count = std::min(sample_count + 1zu, k_window_samples);
    }

    [[nodiscard]] auto snapshot() const -> Snapshot
    {
        Snapshot out{};
        out.sample_count = sample_count;
        if (sample_count == 0zu)
        {
            return out;
        }

        auto ordered = std::vector<f32>{};
        ordered.reserve(sample_count);

        const auto oldest = (next_write + k_window_samples - sample_count) % k_window_samples;
        for (auto i = 0zu; i < sample_count; ++i)
        {
            const auto idx = (oldest + i) % k_window_samples;
            ordered.push_back(samples_ms[idx]);
        }

        auto sum_ms = 0.0;
        for (const auto frame_ms : ordered)
        {
            sum_ms += static_cast<f64>(frame_ms);
        }

        out.window_seconds = sum_ms / 1000.0;
        out.avg_frame_ms = sum_ms / static_cast<f64>(sample_count);
        out.fps = (out.avg_frame_ms > 0.0) ? (1000.0 / out.avg_frame_ms) : 0.0;

        std::sort(ordered.begin(), ordered.end());

        auto percentile = [&](f64 p) -> f64
        {
            Expects(!ordered.empty());
            const auto n = static_cast<f64>(ordered.size());
            const auto rank = std::ceil(p * n);
            const auto rank_1 = std::max(1.0, rank);
            const auto idx = static_cast<usize>(rank_1 - 1.0);
            return static_cast<f64>(ordered[std::min(idx, ordered.size() - 1zu)]);
        };

        out.p50_frame_ms = percentile(0.50);
        out.p95_frame_ms = percentile(0.95);
        out.p99_frame_ms = percentile(0.99);
        out.max_frame_ms = static_cast<f64>(ordered.back());
        out.valid = true;
        return out;
    }
};

auto rolling_frame_stats() -> RollingFrameStats&
{
    static RollingFrameStats stats{};
    return stats;
}

auto reload_scene_for_engine(EngineContext& engine_context, SceneId id) -> void
{
    auto& simulation = engine_context.simulation;
    load_scene(simulation, id);
    engine_context.spit_cube.pending.clear();
    engine_context.spit_cube.has_last_emit = false;
    engine_context.reset_simulation_clock();
}

auto render_physics_debug_window(EngineContext& engine_context) -> void
{
    auto& physics_context = engine_context.simulation.physics;
    auto& gfx_context = engine_context.gfx;
    auto& dbg = gfx_context.phys_debug;

    auto& editor_state = engine_context.simulation.world.editor_state();

    if (gfx_context.request_reveal_physics_debug_window)
    {
        if (const auto* vp = ImGui::GetMainViewport(); vp != nullptr)
        {
            const auto width = std::max(320.0f, std::min(430.0f, vp->WorkSize.x - 32.0f));
            const auto height = std::max(320.0f, std::min(680.0f, vp->WorkSize.y - 48.0f));

            ImGui::SetNextWindowViewport(vp->ID);
            ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + 16.0f, vp->WorkPos.y + 32.0f});
            ImGui::SetNextWindowSize(ImVec2{width, height});
        }
        ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
    }

    ImGui::Begin("Physics Debug");
    if (gfx_context.request_reveal_physics_debug_window)
    {
        ImGui::SetWindowFocus();
        gfx_context.request_reveal_physics_debug_window = false;
    }

    ImGui::TextUnformatted("Simulation");
    auto paused = engine_context.paused;
    if (ImGui::Checkbox("Paused", &paused))
    {
        engine_context.set_paused(paused);
    }

    ImGui::BeginDisabled(!engine_context.paused);
    if (ImGui::Button("Step one frame"))
    {
        engine_context.simulation.physics.step();
        engine_context.simulation.sync_physics_to_world();
    }
    ImGui::EndDisabled();

    auto debug_tracking_enabled = physics_context.debug_tracking_enabled;
    if (ImGui::Checkbox("Track physics debug data", &debug_tracking_enabled))
    {
        physics_context.set_debug_tracking_enabled(debug_tracking_enabled);
        if (!debug_tracking_enabled)
        {
            dbg.enabled = false;
        }
    }
    if (!physics_context.debug_tracking_enabled)
    {
        ImGui::TextUnformatted("Debug tracking disabled for performance.");
    }
    ImGui::TextUnformatted("Emitter: hold F over viewport to spit cubes");

    ImGui::Separator();
    if (!physics_context.debug_tracking_enabled)
    {
        dbg.enabled = false;
    }
    ImGui::BeginDisabled(!physics_context.debug_tracking_enabled);
    ImGui::Checkbox("Enabled", &dbg.enabled);
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::TextUnformatted("Coloring");
    {
        constexpr std::array<const char*, 3> labels{"Diffuse", "Sleep State", "Kinetic Energy"};
        auto mode = static_cast<int>(dbg.color_mode);
        if (ImGui::Combo("Mode", &mode, labels.data(), static_cast<int>(labels.size())))
        {
            dbg.color_mode = static_cast<GfxContext::PhysicsDebugSettings::ColorMode>(mode);
        }
    }

    if (dbg.color_mode == GfxContext::PhysicsDebugSettings::ColorMode::SleepState)
    {
        ImGui::ColorEdit3("Active color", dbg.sleep_active_color.data());
        ImGui::ColorEdit3("Asleep color", dbg.sleep_asleep_color.data());
    }
    else if (dbg.color_mode == GfxContext::PhysicsDebugSettings::ColorMode::KineticEnergy)
    {
        ImGui::Checkbox("Include angular", &dbg.ke_include_angular);
        ImGui::DragFloat("Max KE", &dbg.ke_max, 0.5f, 0.001f, 1e9f, "%.3f");
        ImGui::ColorEdit3("Low", dbg.ke_low_color.data());
        ImGui::ColorEdit3("High", dbg.ke_high_color.data());
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Selected overlays");
    ImGui::Checkbox("Local axes (RGB)", &dbg.show_selected_axes);
    ImGui::Checkbox("Velocity", &dbg.show_selected_velocity);
    ImGui::Checkbox("Angular velocity", &dbg.show_selected_angular_velocity);
    ImGui::Checkbox("Depth test debug lines", &dbg.depth_test);
    ImGui::DragFloat("Axis scale", &dbg.axis_scale, 0.01f, 0.01f, 100.0f);
    ImGui::DragFloat("Velocity scale", &dbg.vel_scale, 0.01f, 0.0f, 100.0f);
    ImGui::DragFloat("Angular velocity scale", &dbg.ang_vel_scale, 0.01f, 0.0f, 100.0f);

    ImGui::Separator();
    ImGui::TextUnformatted("Collisions");
    // $HOOK physics_context.contact_arena.used()
    ImGui::Checkbox("Show contact markers", &dbg.show_contacts);
    ImGui::Checkbox("Show contact normals", &dbg.show_contact_normals);
    ImGui::DragFloat("Contact marker size", &dbg.contact_marker_size, 0.001f, 0.0f, 10.0f);
    ImGui::DragFloat("Contact normal scale", &dbg.contact_normal_scale, 0.01f, 0.0f, 100.0f);

    ImGui::Separator();

    const auto body_count = physics_context.body_count();
    auto dynamic_count = 0zu;
    auto asleep_count = 0zu;
    for (auto i = 0zu; i < body_count; ++i)
    {
        const auto b = physics_context.body(i);
        if (!b.is_static())
        {
            ++dynamic_count;
            if (b.asleep)
            {
                ++asleep_count;
            }
        }
    }

    ImGui::Text(
        "Bodies: %zu (dynamic %zu, asleep %zu)",
        body_count,
        dynamic_count,
        asleep_count
    );
    if (physics_context.debug_tracking_enabled)
    {
        ImGui::Text(
            "Broadphase candidates: %zu",
            physics_context.debug_collision_stats.broadphase_candidates
        );
        ImGui::Text(
            "Narrowphase tests: %zu",
            physics_context.debug_collision_stats.narrowphase_pairs
        );
        const auto n = physics_context.debug_collision_stats.body_count;
        const auto total_pairs = (n > 1zu) ? ((n * (n - 1zu)) / 2zu) : 0zu;
        if (total_pairs > 0zu)
        {
            const auto kept = physics_context.debug_collision_stats.broadphase_candidates;
            const auto pruned = total_pairs > kept ? total_pairs - kept : 0zu;
            const auto pruned_pct =
                (100.0 * static_cast<f64>(pruned)) / static_cast<f64>(total_pairs);
            ImGui::Text("Broadphase pruned: %.1f%%", pruned_pct);
        }
        ImGui::Text("Contacts (last step): %zu", physics_context.debug_contacts.size());
    }
    else
    {
        ImGui::TextUnformatted("Collision stats: tracking disabled");
    }
    ImGui::Text("Selected: %zu", editor_state.selected_ids.size());

    ImGui::Separator();
    if (physics_context.debug_tracking_enabled)
    {
        ImGui::Text(
            "Total kinetic energy: %.3f",
            static_cast<f64>(physics_context.debug_total_kinetic_energy)
        );

        if (!physics_context.debug_total_kinetic_energy_history.empty())
        {
            const auto hz = static_cast<int>(std::lround(1.0 / k_energy_sample_dt));
            const auto seconds = static_cast<int>(k_energy_history_len * k_energy_sample_dt);

            const auto label =
                std::format("Total kinetic energy history (last {}s, {} Hz)", seconds, hz);

            const auto sample_getter = [](void* user_data, int idx) -> float
            {
                const auto* ring = static_cast<const EnergyHistoryRing*>(user_data);
                return ring->at(static_cast<usize>(idx));
            };

            ImGui::PlotLines(
                label.c_str(),
                sample_getter,
                &physics_context.debug_total_kinetic_energy_history,
                static_cast<int>(physics_context.debug_total_kinetic_energy_history.size()),
                0,
                nullptr,
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                ImVec2(0.0f, 90.0f)
            );
        }
        else
        {
            ImGui::TextUnformatted("Energy history: (collecting...)");
        }
    }
    else
    {
        ImGui::TextUnformatted("Energy history: tracking disabled");
    }

    ImGui::End();
}

struct TerminalState
{
    std::vector<std::string> lines{};
    std::array<char, 512> input_buf{};
    bool scroll_to_bottom{false};

    auto add_line(std::string s) -> void
    {
        lines.push_back(std::move(s));
        scroll_to_bottom = true;

        if (lines.size() > k_max_terminal_lines)
        {
            const usize overflow{lines.size() - k_max_terminal_lines};
            using StrDiff = std::vector<std::string>::difference_type;
            lines.erase(lines.begin(), lines.begin() + static_cast<StrDiff>(overflow));
        }
    }
};

auto terminal() -> TerminalState&
{
    static TerminalState t{};
    return t;
}

constexpr auto rgba_u32(u32 rgba) noexcept -> ImVec4
{
    constexpr float inv{1.0f / 255.0f};
    const auto r = static_cast<float>((rgba >> 24) & 0xFFu);
    const auto g = static_cast<float>((rgba >> 16) & 0xFFu);
    const auto b = static_cast<float>((rgba >> 8) & 0xFFu);
    const auto a = static_cast<float>((rgba >> 0) & 0xFFu);
    return ImVec4(r * inv, g * inv, b * inv, a * inv);
}

auto render_terminal_window(EngineContext& engine_context) -> void
{
    TerminalState& t = terminal();

    ImGui::Begin("Terminal");

    const ImVec2 footer_size(0.0f, ImGui::GetFrameHeightWithSpacing());
    if (ImGui::BeginChild(
            "##terminal_scroll",
            ImVec2(0.0f, -footer_size.y),
            true,
            ImGuiWindowFlags_HorizontalScrollbar
        ))
    {
        for (const std::string& line : t.lines)
        {
            ImGui::TextUnformatted(line.c_str());
        }
        if (t.scroll_to_bottom)
        {
            ImGui::SetScrollHereY(1.0f);
            t.scroll_to_bottom = false;
        }
    }
    ImGui::EndChild();

    bool reclaim_focus{false};
    ImGui::Separator();

    const ImGuiInputTextFlags flags{ImGuiInputTextFlags_EnterReturnsTrue};
    if (ImGui::InputText("##terminal_input", t.input_buf.data(), t.input_buf.size(), flags))
    {
        std::string cmd = t.input_buf.data();
        if (cmd == "exit")
        {
            engine_context.deactivate();
        }
        if (!cmd.empty())
        {
            t.add_line("> " + cmd);
            std::println("Terminal input: {}", cmd);
        }
        t.input_buf[0] = '\0';
        reclaim_focus = true;
    }

    if (reclaim_focus)
    {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

}  // namespace

auto ui_log(std::string_view msg) -> void
{
    terminal().add_line(std::string(msg));
}

auto apply_blender_style() -> void
{

    ImGui::StyleColorsDark();

    ImGuiStyle& style{ImGui::GetStyle()};
    ImVec4* c{style.Colors};

    // clang-format off
    struct ColorAssign
    {
        ImGuiCol slot;
        u32      rgba;
    };

    constexpr ColorAssign palette[] = {
        { ImGuiCol_Text,                  0xE6E6E6FF },
        { ImGuiCol_TextDisabled,          0xA6A6A6FF },

        { ImGuiCol_WindowBg,              0x353535FF },
        { ImGuiCol_ChildBg,               0x333333FF },
        { ImGuiCol_PopupBg,               0x252525FF },

        { ImGuiCol_Border,                0x232323FF },
        { ImGuiCol_BorderShadow,          0x00000000 },

        { ImGuiCol_FrameBg,               0x282828FF },
        { ImGuiCol_FrameBgHovered,        0x3A3A3AFF },
        { ImGuiCol_FrameBgActive,         0x424242FF },

        { ImGuiCol_TitleBg,               0x232323FF },
        { ImGuiCol_TitleBgActive,         0x2B2B2BFF },
        { ImGuiCol_TitleBgCollapsed,      0x232323FF },

        { ImGuiCol_MenuBarBg,             0x2E2E2EFF },

        { ImGuiCol_Button,                0x424242FF },
        { ImGuiCol_ButtonHovered,         0x4B4B4BFF },
        { ImGuiCol_ButtonActive,          0x3C3C3CFF },

        { ImGuiCol_Header,                0x314E78FF },
        { ImGuiCol_HeaderHovered,         0x3A3A3AFF },
        { ImGuiCol_HeaderActive,          0x2E3F58FF },

        { ImGuiCol_Separator,             0x2A2A2AFF },
        { ImGuiCol_SeparatorHovered,      0x3A3A3AFF },
        { ImGuiCol_SeparatorActive,       0x4B4B4BFF },

        { ImGuiCol_Tab,                   0x2B2B2BFF },
        { ImGuiCol_TabHovered,            0x3A3A3AFF },
        { ImGuiCol_TabActive,             0x4B4B4BFF },
        { ImGuiCol_TabUnfocused,          0x2B2B2BFF },
        { ImGuiCol_TabUnfocusedActive,    0x4B4B4BFF },

        { ImGuiCol_ScrollbarBg,           0x1E1E1EFF },
        { ImGuiCol_ScrollbarGrab,         0x4B4B4BFF },
        { ImGuiCol_ScrollbarGrabHovered,  0x5A5A5AFF },
        { ImGuiCol_ScrollbarGrabActive,   0x6A6A6AFF },

        { ImGuiCol_CheckMark,             0xFF8500FF },
        { ImGuiCol_SliderGrab,            0xFF8500FF },
        { ImGuiCol_SliderGrabActive,      0xE96A00FF },

        { ImGuiCol_TableRowBg,            0x282828FF },
        { ImGuiCol_TableRowBgAlt,         0x2E2E2EFF },
    };
    // clang-format on

    for (const auto& p : palette)
    {
        c[p.slot] = rgba_u32(p.rgba);
    }

    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.PopupRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.TabRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.ScrollbarRounding = 6.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    style.FramePadding = ImVec2{6.0f, 4.0f};
    style.ItemSpacing = ImVec2{8.0f, 4.0f};
    style.ItemInnerSpacing = ImVec2{6.0f, 4.0f};

    const ImGuiIO& io{ImGui::GetIO()};
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        c[ImGuiCol_WindowBg].w = 1.0f;
    }
}

auto render_imgui_windows(EngineContext& engine_context) -> void
{
    PhysicsContext& physics_context = engine_context.simulation.physics;
    GfxContext& gfx_context = engine_context.gfx;

    auto& world = engine_context.simulation.world;
    auto& editor_state = world.editor_state();
    auto& cam = editor_state.camera();

    {
        auto& simulation = engine_context.simulation;

        ImGui::Begin("Scenes");
        const auto catalog = scene_catalog();
        if (catalog.empty())
        {
            ImGui::TextUnformatted("No scenes are registered.");
            ImGui::End();
        }
        else
        {
            const auto current_index = scene_index(simulation.active_scene).value_or(0zu);
            const auto current_name = scene_name(simulation.active_scene);
            const auto combo_label = std::format("{}##active_scene_combo", current_name);

            if (ImGui::BeginCombo("Active Scene", combo_label.c_str()))
            {
                for (usize i{0zu}; i < catalog.size(); ++i)
                {
                    const auto selected = catalog[i].id == simulation.active_scene;
                    if (ImGui::Selectable(catalog[i].name.data(), selected))
                    {
                        reload_scene_for_engine(engine_context, catalog[i].id);
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button("Reload Active"))
            {
                reload_scene_for_engine(engine_context, simulation.active_scene);
            }

            ImGui::SameLine();
            if (ImGui::Button("Load Default"))
            {
                reload_scene_for_engine(engine_context, k_default_scene);
            }

            ImGui::Separator();
            ImGui::Text("Scene %zu / %zu", current_index + 1zu, catalog.size());
            ImGui::PushTextWrapPos(0.0f);
            const auto desc = scene_description(simulation.active_scene);
            ImGui::TextUnformatted(desc.data(), desc.data() + desc.size());
            ImGui::PopTextWrapPos();

            ImGui::End();
        }
    }

    {
        ImGui::Begin("Info");
        const auto gl_string = [](GLenum name) -> const char*
        { return reinterpret_cast<const char*>(glGetString(name)); };

        ImGui::Text("OpenGL vendor:   %s", gl_string(GL_VENDOR));
        ImGui::Text("OpenGL renderer: %s", gl_string(GL_RENDERER));
        ImGui::Text("OpenGL version:  %s", gl_string(GL_VERSION));
        ImGui::Separator();

        ImGui::Text("Camera:");
        ImGui::Text("  distance: %.3f", static_cast<f64>(cam.distance));
        ImGui::Text("  yaw(deg):  %.2f", static_cast<f64>(glm::degrees(cam.yaw)));
        ImGui::Text("  pitch(deg):%.2f", static_cast<f64>(glm::degrees(cam.pitch)));
        auto pivot_x_d = static_cast<f64>(cam.pivot.x);
        auto pivot_y_d = static_cast<f64>(cam.pivot.y);
        auto pivot_z_d = static_cast<f64>(cam.pivot.z);
        ImGui::Text("  pivot:     (%.2f, %.2f, %.2f)", pivot_x_d, pivot_y_d, pivot_z_d);
        ImGui::Separator();

        ImGui::Text("Frame Counter: %d", gfx_context.frame_count);

        const auto now = Clock::now();
        auto& stats = rolling_frame_stats();
        stats.tick(now);
        const auto snapshot = stats.snapshot();

        const auto runtime_s = std::chrono::duration<f64>(now - gfx_context.run_start).count();
        ImGui::Text("Total Runtime: %.2f s", runtime_s);

        if (snapshot.valid)
        {
            const auto one_percent_low_fps =
                (snapshot.p99_frame_ms > 0.0) ? (1000.0 / snapshot.p99_frame_ms) : 0.0;

            ImGui::Text(
                "Rolling FPS: %.2f (1%% low %.2f) | window %.2fs / %zu frames",
                snapshot.fps,
                one_percent_low_fps,
                snapshot.window_seconds,
                snapshot.sample_count
            );
            ImGui::Text(
                "Frame Time (ms): avg %.2f | p50 %.2f | p95 %.2f | p99 %.2f | max %.2f",
                snapshot.avg_frame_ms,
                snapshot.p50_frame_ms,
                snapshot.p95_frame_ms,
                snapshot.p99_frame_ms,
                snapshot.max_frame_ms
            );
        }
        else
        {
            ImGui::TextUnformatted("Rolling FPS: collecting...");
        }

        ImGui::End();
    }

    {
        auto apply_theme_with_font = [&](const UiTheme& t)
        {
            apply_theme(t);

            auto& imgui_io = ImGui::GetIO();
            auto chosen_font = gfx_context.default_font;

            if (t.font_id)
            {
                if (auto it = gfx_context.fonts_by_id.find(*t.font_id);
                    it != gfx_context.fonts_by_id.end())
                {
                    chosen_font = it->second;
                }
                else
                {
                    std::println(
                        "[UI Theme] Font fallback: theme '{}' uses missing font '{}'. Using "
                        "default.",
                        t.name,
                        *t.font_id
                    );
                }
            }

            imgui_io.FontDefault = chosen_font;
        };

        auto& grid = gfx_context.grid;
        ImGui::Begin("Render");
        const auto& themes = gfx_context.theme_pack.themes;
        if (gfx_context.theme_loaded && !themes.empty())
        {
            auto theme_name = themes[gfx_context.theme_index].name.c_str();
            if (ImGui::BeginCombo("Theme", theme_name))
            {
                for (usize i{0zu}; i < gfx_context.theme_pack.themes.size(); ++i)
                {
                    const auto selected = i == gfx_context.theme_index;
                    if (ImGui::Selectable(gfx_context.theme_pack.themes[i].name.c_str(), selected))
                    {
                        gfx_context.theme_index = i;
                        apply_theme_with_font(gfx_context.theme_pack.themes[i]);
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            ImGui::TextUnformatted("Theme: (not loaded)");
        }
        ImGui::ColorEdit4("Background", gfx_context.background_color.data());
        ImGui::Separator();
        ImGui::Text("Grid");
        ImGui::DragFloat("Fog start", &grid.fog_start, 0.25f, 0.0f, 1e6f);
        ImGui::DragFloat("Fog end", &grid.fog_end, 0.25f, 0.0f, 1e6f);
        ImGui::DragFloat("Minor alpha", &grid.minor_alpha, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Axis alpha", &grid.axis_alpha, 0.01f, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::TextUnformatted("Capture");
        ImGui::DragInt("FPS##capture_fps", &gfx_context.capture_fps, 1.0f, 1, 240);
        {
            const std::string out_dir = gfx_context.capture_output_dir.string();
            ImGui::Text("Output dir: %s", out_dir.c_str());
        }

        if (!gfx_context.recorder.is_recording())
        {
            if (ImGui::Button("Start Recording (F2)"))
            {
                gfx_context.start_recording();
            }
        }
        else
        {
            const std::string out = gfx_context.recorder.output_path.string();
            ImGui::Text("Recording to: %s", out.c_str());
            if (ImGui::Button("Stop Recording (F2)"))
            {
                gfx_context.stop_recording();
            }
        }

        ImGui::End();
    }

    {
        ImGui::Begin("Entity Inspector");
        if (editor_state.active_id)
        {
            auto active_res = world.find(*editor_state.active_id);
            if (!active_res)
            {
                ImGui::Text("Active selection not found.");
                ImGui::End();
                return;
            }
            Entity& o = *active_res;

            const auto physics_index = physics_context.find_body_index(o.id);

            const std::string type_str{to_string(o.type)};

            ImGui::Text("Active : %s [id=%d] [%s]", o.name.c_str(), o.id, type_str.c_str());

            ImGui::Separator();

            ImGui::ColorEdit3("Color", o.color.data());

            if (physics_index)
            {
                auto rb = physics_context.body(*physics_index);
                auto p = rb.position;
                if (ImGui::DragFloat3("Position", &p.x, 0.01f))
                {
                    rb.position = p;
                    (void) world.set_position(o.id, p);
                }

                const Quaternion& ori{rb.orientation};
                const EulerDeg3& rot{glm::degrees(glm::eulerAngles(ori))};
                ImGui::Text(
                    "Orientation (Quaternion) (%.2f,%.2f,%.2f,%.2f)",
                    static_cast<f64>(ori.x),
                    static_cast<f64>(ori.y),
                    static_cast<f64>(ori.z),
                    static_cast<f64>(ori.w)
                );
                ImGui::Text(
                    "Orientation (Degrees) (%.2f°,%.2f°,%.2f°)",
                    static_cast<f64>(rot.x),
                    static_cast<f64>(rot.y),
                    static_cast<f64>(rot.z)
                );
                ImGui::Text(
                    "Scaling (%.2f,%.2f,%.2f)",
                    static_cast<f64>(o.transform.scale.x),
                    static_cast<f64>(o.transform.scale.y),
                    static_cast<f64>(o.transform.scale.z)
                );
                ImGui::Text(
                    "Velocity (%.2f,%.2f,%.2f)",
                    static_cast<f64>(rb.velocity.x),
                    static_cast<f64>(rb.velocity.y),
                    static_cast<f64>(rb.velocity.z)
                );
            }
            else
            {
                auto p = o.transform.position;
                if (ImGui::DragFloat3("Position", &p.x, 0.01f))
                {
                    (void) world.set_position(o.id, p);
                }

                auto s = o.transform.scale;
                if (ImGui::DragFloat3("Scale", &s.x, 0.01f, 0.001f, 1000.0f))
                {
                    s.x = std::max(s.x, 0.001f);
                    s.y = std::max(s.y, 0.001f);
                    s.z = std::max(s.z, 0.001f);
                    (void) world.set_scale(o.id, s);
                }
            }

            auto s = o.transform.scale;
            s.x = std::max(s.x, 0.001f);
            s.y = std::max(s.y, 0.001f);
            s.z = std::max(s.z, 0.001f);
            (void) world.set_scale(o.id, s);
        }
        else
        {
            ImGui::Text("No object selected.");
            ImGui::Text("Left-click objects to select.");
            ImGui::Text("Shift + left-click to multi-select.");
            ImGui::Text("Left-drag in viewport to box-select cubes.");
            ImGui::Text("Hold Ctrl/Cmd while dragging for through-depth selection.");
            ImGui::Text("Press G to grab.");
            ImGui::Text("Middle-mouse drag to orbit.");
        }
        ImGui::End();
    }

    {
        ImGui::Begin("Scene Inspector");
        ImGui::Text("There are %zu entities in the scene", world.entities().size());
        const ImGuiTableFlags flags{
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_ScrollY
        };

        if (ImGui::BeginChild("##scene_list_child", ImVec2(0.0f, 0.0f), true))
        {
            if (ImGui::BeginTable("##scene_list_table", 1, flags))
            {
                if (!world.entities().empty())
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Cubes");
                    ImGui::Separator();

                    for (usize i{0zu}; i < world.entities().size(); ++i)
                    {
                        const auto& entity = world.entities()[i];
                        const auto is_selected = editor_state.is_selected(entity.id);
                        const auto label =
                            std::format("{} [{}]##scene_table_{}", entity.id, "Cube", entity.id);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        const auto is_selectable = ImGui::Selectable(
                            label.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns
                        );
                        if (is_selectable)
                        {
                            const auto shift_down = ImGui::GetIO().KeyShift;
                            if (shift_down)
                            {
                                editor_state.toggle_selection(entity.id);
                            }
                            else
                            {
                                editor_state.select_single(entity.id);
                            }
                        }
                    }
                }
                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
        ImGui::End();
    }
    render_physics_debug_window(engine_context);
    render_terminal_window(engine_context);
}

auto render_menu_bar(EngineContext& engine_context) -> void
{
    auto& gfx_context = engine_context.gfx;

    if (ImGui::BeginMenu("File"))
    {
        if (!gfx_context.recorder.is_recording())
        {
            if (ImGui::MenuItem("Start Recording (F2)"))
            {
                gfx_context.start_recording();
            }
        }
        else
        {
            if (ImGui::MenuItem("Stop Recording (F2)"))
            {
                gfx_context.stop_recording();
            }
        }

        ImGui::Separator();
        if (ImGui::MenuItem("Save Scene"))
        {
            ui_log("Save Scene (not implemented)");
        }
        if (ImGui::MenuItem("Load Scene"))
        {
            ui_log("Load Scene (not implemented)");
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Undo"))
        {
            ui_log("Undo (not implemented)");
        }
        if (ImGui::MenuItem("Redo"))
        {
            ui_log("Redo (not implemented)");
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        if (ImGui::MenuItem("Reveal Physics Debug (F3)"))
        {
            gfx_context.request_reveal_physics_debug_window = true;
        }
        if (ImGui::MenuItem("Reset UI Layout"))
        {
            ImGui::LoadIniSettingsFromMemory("");
            const auto* ini_path = ImGui::GetIO().IniFilename;
            if (ini_path != nullptr && ini_path[0] != '\0')
            {
                ImGui::SaveIniSettingsToDisk(ini_path);
            }
            gfx_context.request_reveal_physics_debug_window = true;
            ui_log("UI layout reset");
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scene"))
    {
        const auto catalog = scene_catalog();
        auto& simulation = engine_context.simulation;

        if (catalog.empty())
        {
            ImGui::TextUnformatted("No scenes are registered.");
        }
        else
        {
            for (usize i{0zu}; i < catalog.size(); ++i)
            {
                const auto selected = catalog[i].id == simulation.active_scene;
                const auto label = std::format("{}##menu_scene_{}", catalog[i].name, i);
                if (ImGui::MenuItem(label.c_str(), nullptr, selected))
                {
                    reload_scene_for_engine(engine_context, catalog[i].id);
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Reload Active"))
            {
                reload_scene_for_engine(engine_context, simulation.active_scene);
            }
            if (ImGui::MenuItem("Load Default"))
            {
                reload_scene_for_engine(engine_context, k_default_scene);
            }
        }

        ImGui::EndMenu();
    }
}

}  // namespace ds_pba
