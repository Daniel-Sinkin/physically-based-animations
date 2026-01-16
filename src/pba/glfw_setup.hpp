// pba/glfw_setup.hpp
#pragma once

#include <expected>

struct GLFWwindow;

namespace ds_pba {

enum class SetupGLFWError {
    InitFailed,
    WindowCreateFailed,
    GladInitFailed
};

std::expected<GLFWwindow *, SetupGLFWError> setup_glfw();

} // namespace ds_pba