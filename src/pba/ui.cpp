// pba/ui.cpp
#include "imgui.h"
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/ui.hpp"
//
#include "pba/engine_context.hpp"

namespace ds_pba
{
namespace
{
struct TerminalState
{
    std::vector<std::string> lines{};
    std::array<char, 512> input_buf{};
    bool scroll_to_bottom{false};

    void add_line(std::string s)
    {
        static constexpr std::size_t k_max_lines = 2000;

        lines.push_back(std::move(s));
        scroll_to_bottom = true;

        if (lines.size() > k_max_lines)
        {
            const std::size_t overflow = lines.size() - k_max_lines;
            using Diff = std::vector<std::string>::difference_type;
            lines.erase(lines.begin(), lines.begin() + static_cast<Diff>(overflow));
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
    constexpr float inv = 1.0f / 255.0f;
    const float r = static_cast<float>((rgba >> 24) & 0xFFu) * inv;
    const float g = static_cast<float>((rgba >> 16) & 0xFFu) * inv;
    const float b = static_cast<float>((rgba >> 8) & 0xFFu) * inv;
    const float a = static_cast<float>((rgba >> 0) & 0xFFu) * inv;
    return ImVec4(r, g, b, a);
}

void render_terminal_window(RenderContext& render_context)
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

    bool reclaim_focus = false;
    ImGui::Separator();

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
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

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    const ImVec4 text = rgba_u32(0xE6E6E6FF);
    const ImVec4 text_disabled = rgba_u32(0xA6A6A6FF);

    const ImVec4 bg_window = rgba_u32(0x353535FF);
    const ImVec4 bg_child = rgba_u32(0x333333FF);
    const ImVec4 bg_popup = rgba_u32(0x252525FF);

    const ImVec4 border = rgba_u32(0x232323FF);

    const ImVec4 frame_bg = rgba_u32(0x282828FF);
    const ImVec4 frame_bg_hover = rgba_u32(0x3A3A3AFF);
    const ImVec4 frame_bg_active = rgba_u32(0x424242FF);

    const ImVec4 title_bg = rgba_u32(0x232323FF);
    const ImVec4 title_bg_active = rgba_u32(0x2B2B2BFF);

    const ImVec4 menubar_bg = rgba_u32(0x2E2E2EFF);

    const ImVec4 button = rgba_u32(0x424242FF);
    const ImVec4 button_hover = rgba_u32(0x4B4B4BFF);
    const ImVec4 button_active = rgba_u32(0x3C3C3CFF);

    const ImVec4 header_selected = rgba_u32(0x314E78FF);
    const ImVec4 header_hover = rgba_u32(0x3A3A3AFF);
    const ImVec4 header_active = rgba_u32(0x2E3F58FF);

    const ImVec4 tab_active = rgba_u32(0x4B4B4BFF);
    const ImVec4 tab_inactive = rgba_u32(0x2B2B2BFF);
    const ImVec4 tab_hover = rgba_u32(0x3A3A3AFF);

    const ImVec4 table_row_bg = rgba_u32(0x282828FF);
    const ImVec4 table_row_bg_alt = rgba_u32(0x2E2E2EFF);

    const ImVec4 accent_orange = rgba_u32(0xFF8500FF);

    c[ImGuiCol_Text] = text;
    c[ImGuiCol_TextDisabled] = text_disabled;

    c[ImGuiCol_WindowBg] = bg_window;
    c[ImGuiCol_ChildBg] = bg_child;
    c[ImGuiCol_PopupBg] = bg_popup;

    c[ImGuiCol_Border] = border;
    c[ImGuiCol_BorderShadow] = rgba_u32(0x00000000);

    c[ImGuiCol_FrameBg] = frame_bg;
    c[ImGuiCol_FrameBgHovered] = frame_bg_hover;
    c[ImGuiCol_FrameBgActive] = frame_bg_active;

    c[ImGuiCol_TitleBg] = title_bg;
    c[ImGuiCol_TitleBgActive] = title_bg_active;
    c[ImGuiCol_TitleBgCollapsed] = title_bg;

    c[ImGuiCol_MenuBarBg] = menubar_bg;

    c[ImGuiCol_Button] = button;
    c[ImGuiCol_ButtonHovered] = button_hover;
    c[ImGuiCol_ButtonActive] = button_active;

    c[ImGuiCol_Header] = header_selected;
    c[ImGuiCol_HeaderHovered] = header_hover;
    c[ImGuiCol_HeaderActive] = header_active;

    c[ImGuiCol_Separator] = rgba_u32(0x2A2A2AFF);
    c[ImGuiCol_SeparatorHovered] = rgba_u32(0x3A3A3AFF);
    c[ImGuiCol_SeparatorActive] = rgba_u32(0x4B4B4BFF);

    c[ImGuiCol_Tab] = tab_inactive;
    c[ImGuiCol_TabHovered] = tab_hover;
    c[ImGuiCol_TabActive] = tab_active;
    c[ImGuiCol_TabUnfocused] = tab_inactive;
    c[ImGuiCol_TabUnfocusedActive] = tab_active;

    c[ImGuiCol_ScrollbarBg] = rgba_u32(0x1E1E1EFF);
    c[ImGuiCol_ScrollbarGrab] = rgba_u32(0x4B4B4BFF);
    c[ImGuiCol_ScrollbarGrabHovered] = rgba_u32(0x5A5A5AFF);
    c[ImGuiCol_ScrollbarGrabActive] = rgba_u32(0x6A6A6AFF);

    c[ImGuiCol_CheckMark] = accent_orange;
    c[ImGuiCol_SliderGrab] = accent_orange;
    c[ImGuiCol_SliderGrabActive] = rgba_u32(0xE96A00FF);

    c[ImGuiCol_TableRowBg] = table_row_bg;
    c[ImGuiCol_TableRowBgAlt] = table_row_bg_alt;

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

    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);

    const ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        c[ImGuiCol_WindowBg].w = 1.0f;
    }
}

void render_imgui_windows(EngineContext& engine_context)
{
    assert(engine_context.physics);
    assert(engine_context.renderer);
    auto& physics_context = *engine_context.physics;
    auto& render_context = *engine_context.renderer;
    assert(render_context.scene_context && "Scene Context of render context not set!");
    auto& scene_context = *render_context.scene_context;

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

        ImGui::Text("Frame Counter: %d", render_context.frame_counter);

        const auto now = std::chrono::steady_clock::now();
        const auto dur = now - render_context.run_start;

        const double seconds = std::chrono::duration<double>(dur).count();

        const double frame_counter_d = static_cast<double>(render_context.frame_counter);
        const double fps = (seconds > 0.0) ? frame_counter_d / seconds : 0.0;

        ImGui::Text("Total Runtime: %.2f s", seconds);
        ImGui::Text("Average FPS: %.2f", fps);

        ImGui::End();
    }

    {
        auto& grid = render_context.grid;
        ImGui::Begin("Render");
        ImGui::ColorEdit4("Background", render_context.background_color.data());
        ImGui::Separator();
        ImGui::Text("Grid");
        ImGui::DragFloat("Fog start", &grid.fog_start, 0.25f, 0.0f, 1e6f);
        ImGui::DragFloat("Fog end", &grid.fog_end, 0.25f, 0.0f, 1e6f);
        ImGui::DragFloat("Minor alpha", &grid.minor_alpha, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Axis alpha", &grid.axis_alpha, 0.01f, 0.0f, 1.0f);
        ImGui::End();
    }

    {
        ImGui::Begin("Object Inspector");
        if (scene_context.selected_index.has_value())
        {
            assert(scene_context.selected_type);

            auto idx = *scene_context.selected_index;
            auto type = *scene_context.selected_type;

            const auto selector = [&](ObjectType type, usize idx) -> Object&
            {
                switch (type)
                {
                    case ObjectType::Cube:
                        assert(idx < scene_context.cube_objects.size());
                        return scene_context.cube_objects[idx];
                    case ObjectType::Sphere:
                        assert(idx < scene_context.sphere_objects.size());
                        return scene_context.sphere_objects[idx];
                    case ObjectType::Hitmarker:
                        assert(idx < scene_context.hitmarker_objects.size());
                        return scene_context.hitmarker_objects[idx];
                }
            };
            Object& o = selector(type, idx);

            std::optional<usize> physics_index{};
            for (usize i{0zu}; i < physics_context.bodies.size(); ++i)
            {
                if (o.id == physics_context.bodies[i].id)
                {
                    physics_index = i;
                    break;
                }
            }

            switch (*scene_context.selected_type)
            {
                case ds_pba::ObjectType::Cube:
                    {
                        ImGui::Text("Selected : %d [Cube]", o.id);
                        break;
                    }
                case ds_pba::ObjectType::Sphere:
                    {
                        ImGui::Text("Selected : %d [Sphere]", o.id);
                        break;
                    }
                case ds_pba::ObjectType::Hitmarker:
                    {
                        ImGui::Text("Selected : %d [Hitmarker]", o.id);
                        break;
                    }
            }

            ImGui::Separator();

            ImGui::ColorEdit3("Color", &o.color.x);

            if (physics_index)
            {
                RigidBody& rb = physics_context.bodies[*physics_index];
                ImGui::DragFloat3("Position", &rb.position.x, 0.01f);
                // ImGui::DragFloat3("Rotation (deg)", &o.transform.rotation_deg.x, 0.25f);
                // ImGui::DragFloat3("Scale", &o.transform.scale.x, 0.01f, 0.001f, 1000.0f);
                ImGui::Text(
                    "Velocity (%.2f,%.2f,%.2f)",
                    static_cast<double>(rb.velocity.x),
                    static_cast<double>(rb.velocity.y),
                    static_cast<double>(rb.velocity.z)
                );
            }
            else
            {  // Non-physical object
                ImGui::DragFloat3("Position", &o.transform.position.x, 0.01f);
                ImGui::DragFloat3("Rotation (deg)", &o.transform.rotation_deg.x, 0.25f);
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
            ImGui::Text("Middle-mouse drag to orbit.");
        }
        ImGui::End();
    }

    {
        ImGui::Begin("Scene Inspector");
        const usize n_obj = scene_context.cube_objects.size() + scene_context.sphere_objects.size();
        ImGui::Text("There are %zu objects in the scene", n_obj);
        if (!scene_context.hitmarker_objects.empty())
        {
            ImGui::Text(
                "There are %zu hitmarkers in the scene", scene_context.hitmarker_objects.size()
            );
        }
        ImGui::Separator();

        const ImGuiTableFlags flags =
            ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_ScrollY;

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

                    for (usize i = 0; i < scene_context.cube_objects.size(); ++i)
                    {
                        const Object& o = scene_context.cube_objects[i];
                        const bool is_selected = scene_context.selected_type == ObjectType::Cube
                                                 && scene_context.selected_index
                                                 && *scene_context.selected_index == i;

                        std::string label = std::format("{} [Cube]##cube_{}", o.id, i);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        bool selectable{ImGui::Selectable(
                            label.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns
                        )};
                        if (selectable)
                        {
                            scene_context.selected_index = i;
                            scene_context.selected_type = ObjectType::Cube;
                        }
                    }
                }

                if (!scene_context.sphere_objects.empty())
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Spheres");
                    ImGui::Separator();

                    for (usize i = 0; i < scene_context.sphere_objects.size(); ++i)
                    {
                        const Object& o = scene_context.sphere_objects[i];
                        const bool is_selected = scene_context.selected_type == ObjectType::Sphere
                                                 && scene_context.selected_index
                                                 && *scene_context.selected_index == i;

                        std::string label = std::format("{} [Sphere]##sphere_{}", o.id, i);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        bool selectable{ImGui::Selectable(
                            label.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns
                        )};
                        if (selectable)
                        {
                            scene_context.selected_index = i;
                            scene_context.selected_type = ObjectType::Sphere;
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
                        const Object& o = scene_context.hitmarker_objects[i];
                        const bool is_selected =
                            scene_context.selected_type == ObjectType::Hitmarker
                            && scene_context.selected_index && *scene_context.selected_index == i;

                        std::string label = std::format("{} [Hitmarker]##hitmarker_{}", o.id, i);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        bool selectable{ImGui::Selectable(
                            label.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns
                        )};
                        if (selectable)
                        {
                            scene_context.selected_index = i;
                            scene_context.selected_type = ObjectType::Hitmarker;
                        }
                    }
                }

                ImGui::EndTable();
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
    render_terminal_window(render_context);
}

void render_menu_bar(RenderContext& render_context)
{
    assert(render_context.scene_context && "RenderContext has no SceneContext!");
    if (ImGui::BeginMenu("File"))
    {
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
