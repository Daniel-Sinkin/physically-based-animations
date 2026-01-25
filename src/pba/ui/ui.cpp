// pba/ui/ui.cpp
//
#include "pba/ui/ui.hpp"
//
#include "glm/gtc/quaternion.hpp"
#include "glm/trigonometric.hpp"
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
#include "pba/engine/engine_context.hpp"
#include "pba/engine/scene_context.hpp"
#include "pba/gfx/gfx_context.hpp"
#include "pba/physics/physics_context.hpp"
//
#include <imgui.h>

namespace ds_pba
{
namespace
{
void render_physics_debug_window(EngineContext& engine_context)
{
    PhysicsContext& physics_context = engine_context.physics;
    GfxContext& gfx_context = engine_context.gfx;
    SceneContext& scene_context = engine_context.scene;

    auto& dbg = gfx_context.phys_debug;

    ImGui::Begin("Physics Debug");

    ImGui::Checkbox("Enabled", &dbg.enabled);

    ImGui::Separator();
    ImGui::TextUnformatted("Coloring");
    {
        const char* labels[] = {"Diffuse", "Sleep State", "Kinetic Energy"};
        int mode = static_cast<int>(dbg.color_mode);
        if (ImGui::Combo("Mode", &mode, labels, IM_ARRAYSIZE(labels)))
        {
            dbg.color_mode = static_cast<GfxContext::PhysicsDebugSettings::ColorMode>(mode);
        }
    }

    if (dbg.color_mode == GfxContext::PhysicsDebugSettings::ColorMode::SleepState)
    {
        ImGui::ColorEdit3("Active color", &dbg.sleep_active_color.x);
        ImGui::ColorEdit3("Asleep color", &dbg.sleep_asleep_color.x);
    }
    else if (dbg.color_mode == GfxContext::PhysicsDebugSettings::ColorMode::KineticEnergy)
    {
        ImGui::Checkbox("Include angular", &dbg.ke_include_angular);
        ImGui::DragFloat("Max KE", &dbg.ke_max, 0.5f, 0.001f, 1e9f, "%.3f");
        ImGui::ColorEdit3("Low", &dbg.ke_low_color.x);
        ImGui::ColorEdit3("High", &dbg.ke_high_color.x);
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
    ImGui::Checkbox("Show contact markers", &dbg.show_contacts);
    ImGui::Checkbox("Show contact normals", &dbg.show_contact_normals);
    ImGui::DragFloat("Contact marker size", &dbg.contact_marker_size, 0.001f, 0.0f, 10.0f);
    ImGui::DragFloat("Contact normal scale", &dbg.contact_normal_scale, 0.01f, 0.0f, 100.0f);

    ImGui::Separator();

    usize dynamic_count{0zu};
    usize asleep_count{0zu};
    for (const RigidBody& b : physics_context.bodies)
    {
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
        physics_context.bodies.size(),
        dynamic_count,
        asleep_count
    );
    ImGui::Text("Contacts (last step): %zu", physics_context.debug_contacts.size());
    ImGui::Text("Selected: %zu", scene_context.selected_ids.size());

    ImGui::Separator();
    ImGui::Text(
        "Total kinetic energy: %.3f",
        static_cast<double>(physics_context.debug_total_kinetic_energy)
    );

    if (!physics_context.debug_total_kinetic_energy_history.empty())
    {
        const int hz = static_cast<int>(std::lround(1.0 / k_energy_sample_dt));
        const int seconds = static_cast<int>(k_energy_history_len * k_energy_sample_dt);

        const auto label =
            std::format("Total kinetic energy history (last {}s, {} Hz)", seconds, hz);

        ImGui::PlotLines(
            label.c_str(),
            physics_context.debug_total_kinetic_energy_history.data(),
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
        ImGui::TextUnformatted("Energy  history: (collecting...)");
    }

    ImGui::End();
}

struct TerminalState
{
    std::vector<std::string> lines{};
    std::array<char, 512> input_buf{};
    bool scroll_to_bottom{false};

    void add_line(std::string s)
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

TerminalState& terminal()
{
    static TerminalState t{};
    return t;
}

constexpr ImVec4 rgba_u32(u32 rgba) noexcept
{
    constexpr float inv{1.0f / 255.0f};
    const auto r = static_cast<float>((rgba >> 24) & 0xFFu);
    const auto g = static_cast<float>((rgba >> 16) & 0xFFu);
    const auto b = static_cast<float>((rgba >> 8) & 0xFFu);
    const auto a = static_cast<float>((rgba >> 0) & 0xFFu);
    return ImVec4(r * inv, g * inv, b * inv, a * inv);
}

void render_terminal_window(GfxContext& render_context)
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
            render_context.deactivate();
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

void ui_log(std::string_view msg)
{
    terminal().add_line(std::string(msg));
}

void apply_blender_style()
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

void render_imgui_windows(EngineContext& engine_context)
{
    PhysicsContext& physics_context = engine_context.physics;
    GfxContext& gfx_context = engine_context.gfx;
    SceneContext& scene_context = engine_context.scene;

    auto object_label = [&](ObjectId id, std::string_view type, usize ui_idx) -> std::string
    {
        auto it = engine_context.obj_name_map.find(id);
        if (it != engine_context.obj_name_map.end())
        {
            return std::format("{} {} [{}]##{}_{}", id, it->second, type, type, ui_idx);
        }
        return std::format("{} [{}]##{}_{}", id, type, type, ui_idx);
    };

    auto& cam = scene_context.camera;
    {
        ImGui::Begin("Info");
        const auto gl_string = [](GLenum name) -> const char*
        { return reinterpret_cast<const char*>(glGetString(name)); };

        ImGui::Text("OpenGL vendor:   %s", gl_string(GL_VENDOR));
        ImGui::Text("OpenGL renderer: %s", gl_string(GL_RENDERER));
        ImGui::Text("OpenGL version:  %s", gl_string(GL_VERSION));
        ImGui::Separator();

        ImGui::Text("Camera:");
        ImGui::Text("  distance: %.3f", static_cast<double>(cam.distance));
        ImGui::Text("  yaw(deg):  %.2f", static_cast<double>(glm::degrees(cam.yaw)));
        ImGui::Text("  pitch(deg):%.2f", static_cast<double>(glm::degrees(cam.pitch)));
        auto pivot_x_d = static_cast<double>(cam.pivot.x);
        auto pivot_y_d = static_cast<double>(cam.pivot.y);
        auto pivot_z_d = static_cast<double>(cam.pivot.z);
        ImGui::Text("  pivot:     (%.2f, %.2f, %.2f)", pivot_x_d, pivot_y_d, pivot_z_d);
        ImGui::Separator();

        ImGui::Text("Frame Counter: %d", gfx_context.frame_count);

        const TimePoint now{std::chrono::steady_clock::now()};
        const auto dur{now - gfx_context.run_start};

        const double seconds{std::chrono::duration<double>(dur).count()};

        const double frame_count_d{static_cast<double>(gfx_context.frame_count)};
        const double fps{(seconds > 0.0) ? frame_count_d / seconds : 0.0};

        ImGui::Text("Total Runtime: %.2f s", seconds);
        ImGui::Text("Average FPS: %.2f", fps);

        ImGui::End();
    }

    {
        auto apply_theme_with_font = [&](const ui_theme::UiTheme& t)
        {
            ui_theme::apply_theme(t);

            ImGuiIO& io = ImGui::GetIO();
            ImFont* chosen = gfx_context.default_font;

            if (t.font_id)
            {
                if (auto it = gfx_context.fonts_by_id.find(*t.font_id);
                    it != gfx_context.fonts_by_id.end())
                {
                    chosen = it->second;
                }
                else
                {
                    std::println(
                        "[UI Theme] Font fallback: theme '{}' requested '{}', not found. Using "
                        "default.",
                        t.name,
                        *t.font_id
                    );
                }
            }

            io.FontDefault = chosen;
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
                    const bool selected{i == gfx_context.theme_index};
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
        ImGui::Begin("Object Inspector");
        if (scene_context.active_id.has_value())
        {
            const ObjectId active_id{*scene_context.active_id};

            auto find_active = [&](ObjectId id) -> std::optional<std::pair<ObjectType, Object*>>
            {
                for (usize i{0zu}; i < scene_context.cube_objects.size(); ++i)
                {
                    Object& o = scene_context.cube_objects[i];
                    if (o.id == id)
                    {
                        return std::pair<ObjectType, Object*>{ObjectType::Cube, &o};
                    }
                }
                for (usize i{0zu}; i < scene_context.sphere_objects.size(); ++i)
                {
                    Object& o = scene_context.sphere_objects[i];
                    if (o.id == id)
                    {
                        return std::pair<ObjectType, Object*>{ObjectType::Sphere, &o};
                    }
                }
                for (usize i{0zu}; i < scene_context.hitmarker_objects.size(); ++i)
                {
                    Object& o = scene_context.hitmarker_objects[i];
                    if (o.id == id)
                    {
                        return std::pair<ObjectType, Object*>{ObjectType::Hitmarker, &o};
                    }
                }
                return std::nullopt;
            };

            auto active_res = find_active(active_id);
            if (!active_res)
            {
                ImGui::Text("Active selection not found.");
                ImGui::End();
                return;
            }

            auto [type, o_ptr] = *active_res;
            Object& o = *o_ptr;

            std::optional<usize> physics_index{};
            for (usize i{0zu}; i < physics_context.bodies.size(); ++i)
            {
                if (o.id == physics_context.bodies[i].id)
                {
                    physics_index = i;
                    break;
                }
            }

            const char* type_str{""};
            switch (type)
            {
                case ds_pba::ObjectType::Cube:
                    type_str = "Cube";
                    break;
                case ds_pba::ObjectType::Sphere:
                    type_str = "Sphere";
                    break;
                case ds_pba::ObjectType::Hitmarker:
                    type_str = "Hitmarker";
                    break;
            }

            if (auto it = engine_context.obj_name_map.find(o.id);
                it != engine_context.obj_name_map.end())
            {
                ImGui::Text("Active : %s [id=%d] [%s]", it->second.c_str(), o.id, type_str);
            }
            else
            {
                ImGui::Text("Active : [id=%d] [%s]", o.id, type_str);
            }

            ImGui::Separator();

            ImGui::ColorEdit3("Color", &o.color.x);

            if (physics_index)
            {
                RigidBody& rb = physics_context.bodies[*physics_index];
                Position3 p = rb.position;
                if (ImGui::DragFloat3("Position", &p.x, 0.01f))
                {
                    gfx_context.set_object_position(o.id, p);
                }

                Quaternion& ori = rb.orientation;
                const EulerDeg3& rot{glm::degrees(glm::eulerAngles(ori))};
                ImGui::Text(
                    "Orientation (Quaternion) (%.2f,%.2f,%.2f,%.2f)",
                    static_cast<double>(ori.x),
                    static_cast<double>(ori.y),
                    static_cast<double>(ori.z),
                    static_cast<double>(ori.w)
                );
                ImGui::Text(
                    "Orientation (Degrees) (%.2f°,%.2f°,%.2f°)",
                    static_cast<double>(rot.x),
                    static_cast<double>(rot.y),
                    static_cast<double>(rot.z)
                );
                ImGui::Text(
                    "Scaling (%.2f,%.2f,%.2f)",
                    static_cast<double>(o.transform.scale.x),
                    static_cast<double>(o.transform.scale.y),
                    static_cast<double>(o.transform.scale.z)
                );
                ImGui::Text(
                    "Velocity (%.2f,%.2f,%.2f)",
                    static_cast<double>(rb.velocity.x),
                    static_cast<double>(rb.velocity.y),
                    static_cast<double>(rb.velocity.z)
                );
            }
            else
            {
                ImGui::DragFloat3("Position", &o.transform.position.x, 0.01f);
                ImGui::DragFloat3("Scale", &o.transform.scale.x, 0.01f, 0.001f, 1000.0f);
            }

            o.transform.scale.x = std::max(o.transform.scale.x, 0.001f);
            o.transform.scale.y = std::max(o.transform.scale.y, 0.001f);
            o.transform.scale.z = std::max(o.transform.scale.z, 0.001f);
        }
        else
        {
            ImGui::Text("No object selected.");
            ImGui::Text("Left-click objects to select.");
            ImGui::Text("Shift + left-click to multi-select.");
            ImGui::Text("Press G to grab.");
            ImGui::Text("Middle-mouse drag to orbit.");
        }
        ImGui::End();
    }

    {
        ImGui::Begin("Scene Inspector");
        const usize n_obj{scene_context.cube_objects.size() + scene_context.sphere_objects.size()};
        ImGui::Text("There are %zu objects in the scene", n_obj);
        if (!scene_context.hitmarker_objects.empty())
        {
            ImGui::Text(
                "There are %zu hitmarkers in the scene", scene_context.hitmarker_objects.size()
            );
        }
        ImGui::Separator();

        const ImGuiTableFlags flags{
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_ScrollY
        };

        if (ImGui::BeginChild("##scene_list_child", ImVec2(0.0f, 0.0f), true))
        {
            if (ImGui::BeginTable("##scene_list_table", 1, flags))
            {
                if (!scene_context.cube_objects.empty())
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Cubes");
                    ImGui::Separator();

                    for (usize i{0zu}; i < scene_context.cube_objects.size(); ++i)
                    {
                        const Object& o{scene_context.cube_objects[i]};
                        const bool is_selected{scene_context.is_selected(o.id)};

                        const std::string label{object_label(o.id, "Cube", i)};

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        const bool selectable{ImGui::Selectable(
                            label.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns
                        )};
                        if (selectable)
                        {
                            const bool shift_down{ImGui::GetIO().KeyShift};
                            if (shift_down)
                            {
                                scene_context.toggle_selection(o.id);
                            }
                            else
                            {
                                scene_context.select_single(o.id);
                            }
                        }
                    }
                }

                if (!scene_context.sphere_objects.empty())
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Spheres");
                    ImGui::Separator();

                    for (usize i{0zu}; i < scene_context.sphere_objects.size(); ++i)
                    {
                        const Object& o{scene_context.sphere_objects[i]};
                        const bool is_selected{scene_context.is_selected(o.id)};

                        const std::string label{object_label(o.id, "Sphere", i)};

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        const bool selectable{ImGui::Selectable(
                            label.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns
                        )};
                        if (selectable)
                        {
                            const bool shift_down{ImGui::GetIO().KeyShift};
                            if (shift_down)
                            {
                                scene_context.toggle_selection(o.id);
                            }
                            else
                            {
                                scene_context.select_single(o.id);
                            }
                        }
                    }
                }
                if (!scene_context.hitmarker_objects.empty())
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Hitmarkers");
                    ImGui::Separator();

                    for (usize i{0zu}; i < scene_context.hitmarker_objects.size(); ++i)
                    {
                        const Object& o{scene_context.hitmarker_objects[i]};
                        const bool is_selected{scene_context.is_selected(o.id)};

                        const std::string label{object_label(o.id, "Hitmarker", i)};

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        const bool selectable{ImGui::Selectable(
                            label.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns
                        )};
                        if (selectable)
                        {
                            const bool shift_down{ImGui::GetIO().KeyShift};
                            if (shift_down)
                            {
                                scene_context.toggle_selection(o.id);
                            }
                            else
                            {
                                scene_context.select_single(o.id);
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
    render_terminal_window(gfx_context);
}

void render_menu_bar(GfxContext& gfx_context)
{
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
            ds_pba::ui_log("Save Scene (not implemented)");
        }
        if (ImGui::MenuItem("Load Scene"))
        {
            ds_pba::ui_log("Load Scene (not implemented)");
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Undo"))
        {
            ds_pba::ui_log("Undo (not implemented)");
        }
        if (ImGui::MenuItem("Redo"))
        {
            ds_pba::ui_log("Redo (not implemented)");
        }
        ImGui::EndMenu();
    }
}

}  // namespace ds_pba
