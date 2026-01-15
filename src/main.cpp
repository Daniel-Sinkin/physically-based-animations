// main.cpp
#include <array>
#include <cstdio>
#include <cstdlib>
#include <print>

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
} // namespace ds_pba

int main() {
    using namespace ds_pba;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::println(stderr, "Failed to initialise GLFW");
        return EXIT_FAILURE;
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
        std::println(stderr, "Failed to instnatiate window");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "Failed to initialize glad.\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

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

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);

    GLuint sprogram = glCreateProgram();
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

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices.data(), GL_STATIC_DRAW);

    GLint pos_loc = glGetAttribLocation(sprogram, "aPos");
    glEnableVertexAttribArray(pos_loc);
    glVertexAttribPointer(pos_loc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    glBindVertexArray(0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    const char *glsl_version = "#version 150";
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        return EXIT_FAILURE;
    }

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

        { // Imgui Windows
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
                ImGui::Begin("Frame Timing");

                const TimePoint t_now = Clock::now();

                const Duration dt = t_now - t_prev;
                const Duration t_elapsed = t_now - t_start;

                t_prev = t_now;

                const f64 dt_sec = dt.count();
                const f64 elapsed_sec = t_elapsed.count();
                const f64 fps = static_cast<double>(total_number_of_frames) / elapsed_sec;

                ImGui::Text("Delta time:   %.6f s", dt_sec);
                ImGui::Text("Elapsed time: %.2f s", elapsed_sec);
                ImGui::Text("FPS:          %.1f", fps);
                // TODO: replace by running count of some number (maybe 60) frame timings for more
                //       accurate framerate instead of considering entire runtime
                ImGui::Text("Frames:       %d", total_number_of_frames);

                ImGui::End();
            }
        }

        ImGui::Render();

        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        { // GL Rendering
            glClearColor(0.10f, 0.12f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(sprogram);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glBindVertexArray(0);
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