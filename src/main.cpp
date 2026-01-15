// main.cpp
#include <array>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <print>
#include <stdlib.h>
#include <tuple>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

#include "types.hpp"

namespace ds_pba {
static void glfw_error_callback(int error, const char *description) {
    std::println(stderr, "GLFW Error {}: {}", error, description);
}

struct RenderSettings {
    ColorRGBAf background_color{0.10f, 0.12f, 0.15f, 1.0f};
};
ds_pba::RenderSettings g_render_settings{};

std::tuple<ShaderProgram, VAO, VBO> setup_shader_program() {
    // Setup Shaderprogram
    const char *vs_src = R"(#version 150
    in vec2 aPos;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
    }
    )";

    const char *fs_src = R"(#version 150
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 0.3, 0.2, 1.0);
    }
    )";

    auto compile_shader = [](GLenum type, const char *src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        return s;
    };

    GLuint vs{compile_shader(GL_VERTEX_SHADER, vs_src)};
    GLuint fs{compile_shader(GL_FRAGMENT_SHADER, fs_src)};

    ShaderProgram sprogram{glCreateProgram()};
    glAttachShader(sprogram, vs);
    glAttachShader(sprogram, fs);
    glLinkProgram(sprogram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    constexpr std::array<f32, 6> triangle_vertices{
        -0.5f,
        -0.5f,
        0.5f,
        -0.5f,
        0.0f,
        0.5f,
    };

    VAO vao{};
    glGenVertexArrays(1, vao.ptr());
    VBO vbo{};
    glGenBuffers(1, vbo.ptr());

    {
        vao.bind();
        vbo.bind();
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices.data(), GL_STATIC_DRAW);

        GLint pos_loc = glGetAttribLocation(sprogram, "aPos");
        glEnableVertexAttribArray(pos_loc);
        glVertexAttribPointer(pos_loc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        vbo.unbind();
        vao.unbind();
    }

    return {sprogram, vao, vbo};
}

enum class SetupGLFWError {
    InitFailed,
    WindowCreateFailed,
    GladInitFailed
};
std::expected<GLFWwindow*, SetupGLFWError> setup_glfw() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        return std::unexpected(SetupGLFWError::InitFailed);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow *window = glfwCreateWindow(
        1600,
        900,
        "Physically Based Animations",
        nullptr,
        nullptr);
    if (!window) {
        return std::unexpected(SetupGLFWError::WindowCreateFailed);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return std::unexpected(SetupGLFWError::GladInitFailed);
    }

    return {window};
}

void render_imgui_windows() {
    {
        ImGui::Begin("Info");
        const auto gl_string = [](GLenum name) -> const char * {
            return reinterpret_cast<const char *>(glGetString(name));
        };

        ImGui::Text("OpenGL vendor:   %s", gl_string(GL_VENDOR));
        ImGui::Text("OpenGL renderer: %s", gl_string(GL_RENDERER));
        ImGui::Text("OpenGL version:  %s", gl_string(GL_VERSION));
        ImGui::End();
    }
    {
        ImGui::Begin("Color Picker");
        ImGui::ColorEdit4("Color", g_render_settings.background_color.data());
        ImGui::End();
    }
}

} // namespace ds_pba


int main() {
    using namespace ds_pba;


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    auto window_res = setup_glfw();
    if(!window_res.has_value()) {
        std::println(stderr, "Failed to setup glfw with error code: {}", static_cast<int>(window_res.error()));
        return EXIT_FAILURE;
    }
    GLFWwindow* window = *window_res;

    const char *glsl_version = "#version 150";
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        return EXIT_FAILURE;
    }

    auto [sprogram, vao, vbo] = setup_shader_program();


    RenderSettings render_settings{};
    auto framebuffer_callback = [](GLFWwindow *window, int width, int height) -> void {
        glViewport(0, 0, width, height);
    };
    glfwSetFramebufferSizeCallback(window, framebuffer_callback);

    int total_number_of_frames = 0;
    const TimePoint t_start = Clock::now();
    TimePoint t_prev = t_start;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        { // Input Handling
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        { // ImGui
            render_imgui_windows();
            ImGui::Render();
        }
        { // Rendering
            auto bg = render_settings.background_color;
            glClearColor(bg.r(), bg.g(), bg.b(), bg.a());
            glClear(GL_COLOR_BUFFER_BIT);

            sprogram.bind();
            vao.bind();
            glDrawArrays(GL_TRIANGLES, 0, 3);
            VAO::unbind();
        }
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        ++total_number_of_frames;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}