// pba/render/gfx_context.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/gfx/gl.hpp"
#include "pba/gfx/gl_types.hpp"
#include "pba/gfx/video_recorder.hpp"
#include "pba/ui/ui.hpp"
#include "pba/util/shutdown.hpp"
//
#include <atomic>
#include <imgui.h>
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <json.hpp>

ds_pba::GfxContext::~GfxContext()
{
    shutdown();
}

void ds_pba::GfxContext::shutdown()
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

        invalidate_uniform_cache_for_program(grid_prog.handle());
        invalidate_uniform_cache_for_program(obj_prog.handle());
        invalidate_uniform_cache_for_program(outline_prog.handle());
        invalidate_uniform_cache_for_program(pivot_prog.handle());

        glDeleteProgram(grid_prog.id);
        glDeleteProgram(obj_prog.id);
        glDeleteProgram(outline_prog.id);
        glDeleteProgram(pivot_prog.id);

        grid_prog.id = 0;
        obj_prog.id = 0;
        outline_prog.id = 0;
        pivot_prog.id = 0;

        auto destroy_mesh = [](GLMesh& m)
        {
            if (m.vbo.id != 0)
            {
                glDeleteBuffers(1, &m.vbo.id);
                m.vbo.id = 0;
            }
            if (m.vao.id != 0)
            {
                glDeleteVertexArrays(1, &m.vao.id);
                m.vao.id = 0;
            }
            m.vertex_count = 0;
        };

        destroy_mesh(cube_mesh);
        destroy_mesh(sphere_mesh);
        destroy_mesh(grid_mesh);
        destroy_mesh(marble_bust_mesh);
        destroy_mesh(pyramid_mesh);
        destroy_mesh(cylinder_mesh);

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

bool ds_pba::GfxContext::is_active() const
{
    return (window != nullptr) && !glfwWindowShouldClose(window) && is_active_;
}

void ds_pba::GfxContext::request_close() noexcept
{
    is_active_ = false;
    if (window)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void ds_pba::GfxContext::step()
{

    using namespace ds_pba;
    assert(scene_context && "Scene Context not set for GfxContext");

    if (!is_active())
    {
        return;
    }
    {  // See shutdown.hpp for details on our signal handling
        if (ds_pba::g_request_close_sig)
        {
            ds_pba::g_request_close.store(true, std::memory_order_relaxed);
            ds_pba::g_request_close_sig = 0;
        }
        if (ds_pba::g_request_close.load(std::memory_order_relaxed))
        {
            request_close();
        }
    }

    glfwPollEvents();

    const ImGuiIO& io{ImGui::GetIO()};
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar())
    {
        render_menu_bar(*this);
        ImGui::EndMainMenuBar();
    }

    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    viewport_window();

    const bool f1_down{glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS};
    if (f1_down && !prev_f1)
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
    prev_f1 = f1_down;

    const bool f2_down{glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS};
    if (f2_down && !prev_f2)
    {
        toggle_recording();
    }
    prev_f2 = f2_down;

    const auto mouse_x = static_cast<f64>(io.MousePos.x);
    const auto mouse_y = static_cast<f64>(io.MousePos.y);

    const bool left_down{glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS};
    const bool middle_down{glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS};
    const bool right_down{glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS};
    const bool g_down{glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS};
    const bool esc_down{glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS};
    const bool enter_down{glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS};
    const bool kp_enter_down{glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS};

    const bool g_pressed{g_down && !prev_g};

    const bool esc_pressed{esc_down && !prev_esc};
    const bool enter_pressed{enter_down && !prev_enter};
    const bool kp_enter_pressed{kp_enter_down && !prev_kp_enter};

    // Grab mode takes priority
    if (grab.active)
    {
        update_grab(mouse_x, mouse_y);
        if (ImGui::IsKeyPressed(ImGuiKey_X))
        {
            set_grab_constraint(GrabConstraint::X);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Y))
        {
            set_grab_constraint(GrabConstraint::Y);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            set_grab_constraint(GrabConstraint::Z);
        }

        const bool confirm{(left_down && !prev_left) || enter_pressed || kp_enter_pressed};
        const bool cancel{(right_down && !prev_right) || esc_pressed};

        if (cancel)
        {
            cancel_grab();
        }
        else if (confirm)
        {
            confirm_grab();
        }
    }
    else
    {
        if (viewport_image_hovered && g_pressed && !io.WantCaptureKeyboard)
        {
            begin_grab(mouse_x, mouse_y);
        }

        if (esc_pressed)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        if (viewport_image_hovered)
        {
            hover_interaction(mouse_x, mouse_y, left_down, middle_down, right_down);
        }
    }

    prev_left = left_down;
    prev_middle = middle_down;
    prev_right = right_down;

    prev_g = g_down;

    prev_esc = esc_down;
    prev_enter = enter_down;
    prev_kp_enter = kp_enter_down;

    prev_mx = mouse_x;
    prev_my = mouse_y;

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
