// pba/editor/editor_input.cpp
#include "pba/editor/editor_input.hpp"
//
#include <GLFW/glfw3.h>
#include <gsl/pointers>
#include <imgui.h>

namespace ds_pba
{

void EditorInput::update(not_null<GLFWwindow*> w) noexcept
{
    for (usize i{0zu}; i < k_editor_key_count; ++i)
    {
        auto& s = keys[i];
        s.prev = s.down;

        const auto k = static_cast<EditorKey>(i);
        const int glfw_k{to_glfw_key(k)};
        if (k == EditorKey::Shift)
        {
            s.down = (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                     || (glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
        }
        else
        {
            s.down = (glfw_k >= 0) && (glfwGetKey(w, glfw_k) == GLFW_PRESS);
        }
    }

    mouse_left.prev = mouse_left.down;
    mouse_middle.prev = mouse_middle.down;
    mouse_right.prev = mouse_right.down;

    mouse_left.down = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    mouse_middle.down = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    mouse_right.down = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

    prev_win_mouse_x = win_mouse_x;
    prev_win_mouse_y = win_mouse_y;
    glfwGetCursorPos(w, &win_mouse_x, &win_mouse_y);
}

bool EditorInput::key_down(EditorKey k) const noexcept
{
    return keys[static_cast<usize>(k)].down;
}

bool EditorInput::key_pressed(EditorKey k) const noexcept
{
    return keys[static_cast<usize>(k)].pressed();
}

void EditorInput::sync_imgui_mouse() noexcept
{
    const ImGuiIO& io = ImGui::GetIO();

    prev_ui_mouse_x = ui_mouse_x;
    prev_ui_mouse_y = ui_mouse_y;

    ui_mouse_x = static_cast<f64>(io.MousePos.x);
    ui_mouse_y = static_cast<f64>(io.MousePos.y);
}

}  // namespace ds_pba
