// pba/glfw_setup.cpp
#include "glfw_setup.hpp"

#include <cstdlib>
#include <print>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace ds_pba {

namespace {
void glfw_error_callback(int error, const char *description) {
    std::println(stderr, "GLFW Error {}: {}", error, description);
}
} // namespace

std::expected<GLFWwindow *, SetupGLFWError> setup_glfw() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        return std::unexpected(SetupGLFWError::InitFailed);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow *window = glfwCreateWindow(1600, 900, "Physically Based Animations", nullptr, nullptr);
    if (!window) {
        return std::unexpected(SetupGLFWError::WindowCreateFailed);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return std::unexpected(SetupGLFWError::GladInitFailed);
    }

    return {window};
}

} // namespace ds_pba