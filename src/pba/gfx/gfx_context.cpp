// pba/gfx/gfx_context.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/editor/editor_input.hpp"
#include "pba/gfx/video_recorder.hpp"
#include "pba/ui/ui.hpp"
#include "pba/util/shutdown.hpp"
//
#include <atomic>
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <imgui.h>
#include <json.hpp>

namespace ds_pba
{
GfxContext::~GfxContext()
{
    shutdown();
}

auto GfxContext::shutdown() -> void
{
    if (recorder.is_recording())
    {
        recorder.stop();
    }

    if (loaded_glad)
    {
        if (window)
        {
            glfwMakeContextCurrent(window);
        }

        viewport_fbo.destroy();

        shader_programs.destroy();
        meshes.destroy();
        textures.destroy();

        loaded_glad = false;
    }
    if (initialised_imgui)
    {

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        initialised_imgui = false;
    }
    if (window_created && window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
        window_created = false;
    }
    if (initialised_glfw)
    {
        glfwTerminate();
        initialised_glfw = false;
    }
}

auto GfxContext::step(EditorInput& input) -> void
{
    Expects(engine_context && "Scene Context not set for GfxContext");
    using namespace ds_pba;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const ImGuiIO& io{ImGui::GetIO()};
    imgui_uses_keyboard = io.WantCaptureKeyboard;
    imgui_uses_mouse = io.WantCaptureMouse;

    if (ImGui::BeginMainMenuBar())
    {
        render_menu_bar(*this);
        ImGui::EndMainMenuBar();
    }

    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    viewport_window();

    if (input.key_pressed(EditorKey::F1))
    {  // Switch pivot off and on
        if (pivot_active)
        {
            ui_log("Deactivated the pivot");
            pivot_active = false;
        }
        else
        {
            ui_log("Activated the pivot");
            pivot_active = true;
        }
    }
    if (input.key_pressed(EditorKey::F2))
    {
        toggle_recording();
    }

    if (editor.grab.active)
    {
        update_grab(input);
        if (input.key_pressed(EditorKey::X))
        {
            set_grab_constraint(EditorState::GrabConstraint::X);
        }
        else if (input.key_pressed(EditorKey::Y))
        {
            set_grab_constraint(
                k_is_german_keyboard ? EditorState::GrabConstraint::Z
                                     : EditorState::GrabConstraint::Y
            );
        }
        else if (input.key_pressed(EditorKey::Z))
        {
            set_grab_constraint(
                k_is_german_keyboard ? EditorState::GrabConstraint::Y
                                     : EditorState::GrabConstraint::Z
            );
        }

        if (input.mouse_right.pressed() || input.key_pressed(EditorKey::Escape))
        {
            cancel_grab();
        }
        else if (input.mouse_left.pressed() || input.key_pressed(EditorKey::Enter))
        {
            confirm_grab();
        }
    }
    else
    {
        if (viewport_image_hovered && input.key_pressed(EditorKey::G) && !io.WantCaptureKeyboard)
        {
            begin_grab(input);
        }

        if (input.key_pressed(EditorKey::Escape))
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        if (viewport_image_hovered)
        {
            hover_interaction(input);
        }
    }

    render_imgui_windows(*engine_context);

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    if (recorder.is_recording())
    {
        const int w{viewport_fbo.width};
        const int h{viewport_fbo.height};

        if (!viewport_fb_rect_valid)
        {
            ui_log("Recording stopped: viewport became invalid");
            stop_recording();
        }
        else if (w != recorder.width || h != recorder.height)
        {
            ui_log("Recording stopped: viewport size changed during recording");
            stop_recording();
        }
        else
        {
            if (!capture_viewport_rgba8(capture_rgba))
            {
                ui_log("Recording stopped: framebuffer readback failed");
                stop_recording();
            }
            else
            {
                const std::span<const u8> frame{capture_rgba.data(), capture_rgba.size()};
                if (!recorder.write_frame(frame))
                {
                    ui_log("Recording stopped: failed to write frame to ffmpeg");
                    stop_recording();
                }
            }
        }
    }

    glfwSwapBuffers(window);
    ++frame_count;
}
}  // namespace ds_pba
