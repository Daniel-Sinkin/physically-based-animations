// pba/ui.cpp
#include "ui.hpp"

#include <algorithm>
#include <string>

#include "camera.hpp"
#include "imgui.h"
#include "render_settings.hpp"
#include "scene_context.hpp"

namespace ds_pba {

void render_imgui_windows(SceneContext &ctx, std::optional<usize> &selected_index, int frame_counter) {
    auto &cam = ctx.camera;

    {
        ImGui::Begin("Info");
        const auto gl_string = [](GLenum name) -> const char * {
            return reinterpret_cast<const char *>(glGetString(name));
        };

        ImGui::Text("OpenGL vendor:   %s", gl_string(GL_VENDOR));
        ImGui::Text("OpenGL renderer: %s", gl_string(GL_RENDERER));
        ImGui::Text("OpenGL version:  %s", gl_string(GL_VERSION));
        ImGui::Separator();

        ImGui::Text("Camera:");
        ImGui::Text("  distance: %.3f", static_cast<double>(cam.distance));
        ImGui::Text("  yaw(deg):  %.2f", static_cast<double>(glm::degrees(cam.yaw)));
        ImGui::Text("  pitch(deg):%.2f", static_cast<double>(glm::degrees(cam.pitch)));
        ImGui::Text("  pivot:     (%.2f, %.2f, %.2f)", static_cast<double>(cam.pivot.x), static_cast<double>(cam.pivot.y), static_cast<double>(cam.pivot.z));
        ImGui::End();

        ImGui::Text("Frame Counter: %d", frame_counter);
    }

    {
        ImGui::Begin("Render");
        ImGui::ColorEdit4("Background", g_render_settings.background_color.data());
        ImGui::Separator();
        ImGui::Text("Grid");
        ImGui::DragFloat("Fog start", &g_render_settings.grid.fog_start, 0.25f, 0.0f, 1e6f);
        ImGui::DragFloat("Fog end", &g_render_settings.grid.fog_end, 0.25f, 0.0f, 1e6f);
        ImGui::DragFloat("Minor alpha", &g_render_settings.grid.minor_alpha, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Axis alpha", &g_render_settings.grid.axis_alpha, 0.01f, 0.0f, 1.0f);
        ImGui::End();
    }

    {
        ImGui::Begin("Object Inspector");
        if (selected_index.has_value() && *selected_index < ctx.cube_objects.size()) {
            Object &o = ctx.cube_objects[*selected_index];
            ImGui::Text("Selected: %s", o.name.c_str());
            ImGui::Separator();

            ImGui::ColorEdit3("Color", &o.color.x);
            ImGui::DragFloat3("Position", &o.transform.position.x, 0.01f);
            ImGui::DragFloat3("Rotation (deg)", &o.transform.rotation_deg.x, 0.25f);
            ImGui::DragFloat3("Scale", &o.transform.scale.x, 0.01f, 0.001f, 1000.0f);

            o.transform.scale.x = std::max(o.transform.scale.x, 0.001f);
            o.transform.scale.y = std::max(o.transform.scale.y, 0.001f);
            o.transform.scale.z = std::max(o.transform.scale.z, 0.001f);
        } else {
            ImGui::Text("No object selected.");
            ImGui::Text("Left-click objects to select.");
            ImGui::Text("Middle-mouse drag to orbit.");
        }
        ImGui::End();
    }

    {
        ImGui::Begin("Scene Inspector");
        ImGui::Text("There are %zu objects in the scene", ctx.cube_objects.size());
        for (usize i = 0; i < ctx.cube_objects.size(); ++i) {
            const Object& o = ctx.cube_objects[i];

            const bool is_selected = selected_index.has_value() && (*selected_index == i);

            std::string label = std::to_string(i) + " - " + o.name + "##" + std::to_string(i);

            if (ImGui::Selectable(label.c_str(), is_selected)) {
                selected_index = i;
            }
        }

        ImGui::End();
    }
}

} // namespace ds_pba