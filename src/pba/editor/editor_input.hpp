// pba/editor/editor_input.hpp
#pragma once

#include "pba/core/core_types.hpp"
//
#include <array>
#include <utility>
//
#include <GLFW/glfw3.h>

struct GLFWwindow;

namespace ds_pba
{

enum class EditorKey : u8
{
    // clang-format off
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    Enter,
    KpEnter,
    Space,
    Escape,
    Backspace,
    Delete,
    Tab,
    Shift,

    F1, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,

    Count
    // clang-format on
};
constexpr auto k_editor_key_count = static_cast<usize>(std::to_underlying(EditorKey::Count));

constexpr int to_glfw_key(EditorKey k) noexcept
{
    // clang-format off
    switch (k)
    {
        case EditorKey::A: return GLFW_KEY_A;
        case EditorKey::B: return GLFW_KEY_B;
        case EditorKey::C: return GLFW_KEY_C;
        case EditorKey::D: return GLFW_KEY_D;
        case EditorKey::E: return GLFW_KEY_E;
        case EditorKey::F: return GLFW_KEY_F;
        case EditorKey::G: return GLFW_KEY_G;
        case EditorKey::H: return GLFW_KEY_H;
        case EditorKey::I: return GLFW_KEY_I;
        case EditorKey::J: return GLFW_KEY_J;
        case EditorKey::K: return GLFW_KEY_K;
        case EditorKey::L: return GLFW_KEY_L;
        case EditorKey::M: return GLFW_KEY_M;
        case EditorKey::N: return GLFW_KEY_N;
        case EditorKey::O: return GLFW_KEY_O;
        case EditorKey::P: return GLFW_KEY_P;
        case EditorKey::Q: return GLFW_KEY_Q;
        case EditorKey::R: return GLFW_KEY_R;
        case EditorKey::S: return GLFW_KEY_S;
        case EditorKey::T: return GLFW_KEY_T;
        case EditorKey::U: return GLFW_KEY_U;
        case EditorKey::V: return GLFW_KEY_V;
        case EditorKey::W: return GLFW_KEY_W;
        case EditorKey::X: return GLFW_KEY_X;
        case EditorKey::Y: return GLFW_KEY_Y;
        case EditorKey::Z: return GLFW_KEY_Z;

        case EditorKey::Enter:     return GLFW_KEY_ENTER;
        case EditorKey::KpEnter:   return GLFW_KEY_KP_ENTER;
        case EditorKey::Space:     return GLFW_KEY_SPACE;
        case EditorKey::Escape:    return GLFW_KEY_ESCAPE;
        case EditorKey::Backspace: return GLFW_KEY_BACKSPACE;
        case EditorKey::Delete:    return GLFW_KEY_DELETE;
        case EditorKey::Tab:       return GLFW_KEY_TAB;

        case EditorKey::Shift:     return -1;
        case EditorKey::F1:  return GLFW_KEY_F1;
        case EditorKey::F2:  return GLFW_KEY_F2;
        case EditorKey::F3:  return GLFW_KEY_F3;
        case EditorKey::F4:  return GLFW_KEY_F4;
        case EditorKey::F5:  return GLFW_KEY_F5;
        case EditorKey::F6:  return GLFW_KEY_F6;
        case EditorKey::F7:  return GLFW_KEY_F7;
        case EditorKey::F8:  return GLFW_KEY_F8;
        case EditorKey::F9:  return GLFW_KEY_F9;
        case EditorKey::F10: return GLFW_KEY_F10;
        case EditorKey::F11: return GLFW_KEY_F11;
        case EditorKey::F12: return GLFW_KEY_F12;

        default: return -1;
    }
    // clang-format on
}

struct ButtonState
{
    bool down{false};
    bool prev{false};

    [[nodiscard]] bool pressed() const noexcept
    {
        return down && !prev;
    }
    [[nodiscard]] bool released() const noexcept
    {
        return !down && prev;
    }
};

struct EditorInput
{
    std::array<ButtonState, k_editor_key_count> keys{};
    ButtonState mouse_left{};
    ButtonState mouse_middle{};
    ButtonState mouse_right{};

    f64 win_mouse_x{};
    f64 win_mouse_y{};
    f64 prev_win_mouse_x{};
    f64 prev_win_mouse_y{};

    [[nodiscard]] f64 win_dx() const noexcept
    {
        return win_mouse_x - prev_win_mouse_x;
    }
    [[nodiscard]] f64 win_dy() const noexcept
    {
        return win_mouse_y - prev_win_mouse_y;
    }

    f64 ui_mouse_x{};
    f64 ui_mouse_y{};
    f64 prev_ui_mouse_x{};
    f64 prev_ui_mouse_y{};

    [[nodiscard]] f64 ui_dx() const noexcept
    {
        return ui_mouse_x - prev_ui_mouse_x;
    }
    [[nodiscard]] f64 ui_dy() const noexcept
    {
        return ui_mouse_y - prev_ui_mouse_y;
    }

    void update(not_null<GLFWwindow*> window) noexcept;
    void sync_imgui_mouse() noexcept;

    [[nodiscard]] bool key_down(EditorKey k) const noexcept;
    [[nodiscard]] bool key_pressed(EditorKey k) const noexcept;
};

}  // namespace ds_pba
