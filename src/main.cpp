// main.cpp
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <optional>
#include <print>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

#include "types.hpp"

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ds_pba {

static void glfw_error_callback(int error, const char *description) {
    std::println(stderr, "GLFW Error {}: {}", error, description);
}

struct RenderSettings {
    ColorRGBAf background_color{0.255f, 0.255f, 0.255f, 1.0f};

    struct GridSettings {
        int half_extent_lines = 30; // lines from [-N..N]
        f32 spacing = 1.0f;
        f32 fog_start = 12.0f; // start fading
        f32 fog_end = 30.0f;   // fully faded
        f32 minor_alpha = 0.35f;
        f32 axis_alpha = 0.95f;
    } grid{};
};

static RenderSettings g_render_settings{};

struct Transform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation_deg{0.0f, 0.0f, 0.0f}; // x,y,z degrees
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    [[nodiscard]] glm::mat4 model_matrix() const {
        glm::mat4 M(1.0f);
        M = glm::translate(M, position);
        // Match previous behavior: Rz * Ry * Rx
        M = glm::rotate(M, glm::radians(rotation_deg.z), glm::vec3(0, 0, 1));
        M = glm::rotate(M, glm::radians(rotation_deg.y), glm::vec3(0, 1, 0));
        M = glm::rotate(M, glm::radians(rotation_deg.x), glm::vec3(1, 0, 0));
        M = glm::scale(M, scale);
        return M;
    }
};

struct Object {
    std::string name;
    Transform transform{};
    glm::vec3 color{0.8f, 0.8f, 0.8f};
};

struct Ray {
    glm::vec3 origin{};
    glm::vec3 dir{}; // normalized
};

struct OrbitCamera {
    glm::vec3 pivot{0, 0, 0};
    f32 distance = 8.0f;

    // yaw around +Z, pitch up/down (radians)
    f32 yaw = glm::radians(45.0f);
    f32 pitch = glm::radians(25.0f);

    f32 fov_y = glm::radians(45.0f);
    f32 z_near = 0.1f;
    f32 z_far = 250.0f;

    [[nodiscard]] glm::vec3 position() const {
        const f32 cp = std::cos(pitch);
        const f32 sp = std::sin(pitch);
        const f32 cy = std::cos(yaw);
        const f32 sy = std::sin(yaw);
        const glm::vec3 offset{
            distance * cp * cy,
            distance * cp * sy,
            distance * sp};
        return pivot + offset;
    }

    [[nodiscard]] glm::mat4 view_matrix() const {
        return glm::lookAt(position(), pivot, glm::vec3(0, 0, 1));
    }

    [[nodiscard]] glm::mat4 proj_matrix(f32 aspect) const {
        return glm::perspective(fov_y, aspect, z_near, z_far);
    }
};

static inline f32 max3(f32 a, f32 b, f32 c) { return std::max(a, std::max(b, c)); }

static inline bool intersect_sphere(const Ray &ray, const glm::vec3 &center, f32 radius, f32 &t_hit) {
    // Ray-sphere intersection (nearest positive t)
    const glm::vec3 oc = ray.origin - center;
    const f32 b = 2.0f * glm::dot(oc, ray.dir);
    const f32 c = glm::dot(oc, oc) - radius * radius;
    const f32 disc = b * b - 4.0f * c;
    if (disc < 0.0f)
        return false;

    const f32 s = std::sqrt(disc);
    const f32 t0 = (-b - s) * 0.5f;
    const f32 t1 = (-b + s) * 0.5f;

    const f32 t = (t0 > 0.0f) ? t0 : ((t1 > 0.0f) ? t1 : -1.0f);
    if (t <= 0.0f)
        return false;

    t_hit = t;
    return true;
}

static Ray ray_from_mouse(
    GLFWwindow *window,
    f64 mx_window,
    f64 my_window,
    const glm::mat4 &V,
    const glm::mat4 &P) {
    // Handle HiDPI: cursor is in window coords; NDC should use framebuffer coords.
    int ww = 1, wh = 1;
    int fbw = 1, fbh = 1;
    glfwGetWindowSize(window, &ww, &wh);
    glfwGetFramebufferSize(window, &fbw, &fbh);

    const f32 sx = (ww > 0) ? (static_cast<f32>(fbw) / static_cast<f32>(ww)) : 1.0f;
    const f32 sy = (wh > 0) ? (static_cast<f32>(fbh) / static_cast<f32>(wh)) : 1.0f;

    const f32 mx = static_cast<f32>(mx_window) * sx;
    const f32 my = static_cast<f32>(my_window) * sy;

    // NDC: x,y in [-1,1], OpenGL z in [-1,1]
    const f32 x_ndc = (fbw > 0) ? (2.0f * mx / static_cast<f32>(fbw) - 1.0f) : 0.0f;
    const f32 y_ndc = (fbh > 0) ? (1.0f - 2.0f * my / static_cast<f32>(fbh)) : 0.0f;

    const glm::mat4 invPV = glm::inverse(P * V);

    glm::vec4 near_ndc(x_ndc, y_ndc, -1.0f, 1.0f);
    glm::vec4 far_ndc(x_ndc, y_ndc, 1.0f, 1.0f);

    glm::vec4 near_w = invPV * near_ndc;
    glm::vec4 far_w = invPV * far_ndc;
    near_w /= near_w.w;
    far_w /= far_w.w;

    Ray r{};
    r.origin = glm::vec3(near_w);
    r.dir = glm::normalize(glm::vec3(far_w - near_w));
    return r;
}

// ---------------- GL Helpers ----------------

struct GLMesh {
    VAO vao{};
    VBO vbo{};
    GLsizei vertex_count{};
};

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint log_len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<usize>(std::max(1, log_len)), '\0');
        glGetShaderInfoLog(s, log_len, nullptr, log.data());
        std::println(stderr, "Shader compile failed:\n{}", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static ShaderProgram create_program(const char *vs_src, const char *fs_src) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    if (vs == 0 || fs == 0) {
        if (vs) {
            glDeleteShader(vs);
        }
        if (fs) {
            glDeleteShader(fs);
        }
        return ShaderProgram{0};
    }

    ShaderProgram prog{glCreateProgram()};
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint log_len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<usize>(std::max(1, log_len)), '\0');
        glGetProgramInfoLog(prog, log_len, nullptr, log.data());
        std::println(stderr, "Program link failed:\n{}", log);
        glDeleteProgram(prog.id);
        prog.id = 0;
    }
    return prog;
}

static inline void set_uniform_mat4(GLuint program, const char *name, const glm::mat4 &m) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

static inline void set_uniform_vec3(GLuint program, const char *name, const glm::vec3 &v) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform3f(loc, v.x, v.y, v.z);
}

static inline void set_uniform_float(GLuint program, const char *name, f32 v) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform1f(loc, v);
}

static inline void set_uniform_bool(GLuint program, const char *name, bool v) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0)
        glUniform1i(loc, v ? 1 : 0);
}

// ---------------- Mesh Creation ----------------

static GLMesh create_cube_mesh() {
    struct V {
        f32 px, py, pz;
        f32 nx, ny, nz;
    };

    static constexpr std::array<V, 36> verts = {
        // +Z
        V{-0.5f, -0.5f, 0.5f, 0, 0, 1},
        V{0.5f, -0.5f, 0.5f, 0, 0, 1},
        V{0.5f, 0.5f, 0.5f, 0, 0, 1},
        V{-0.5f, -0.5f, 0.5f, 0, 0, 1},
        V{0.5f, 0.5f, 0.5f, 0, 0, 1},
        V{-0.5f, 0.5f, 0.5f, 0, 0, 1},
        // -Z
        V{-0.5f, -0.5f, -0.5f, 0, 0, -1},
        V{0.5f, 0.5f, -0.5f, 0, 0, -1},
        V{0.5f, -0.5f, -0.5f, 0, 0, -1},
        V{-0.5f, -0.5f, -0.5f, 0, 0, -1},
        V{-0.5f, 0.5f, -0.5f, 0, 0, -1},
        V{0.5f, 0.5f, -0.5f, 0, 0, -1},
        // +X
        V{0.5f, -0.5f, -0.5f, 1, 0, 0},
        V{0.5f, 0.5f, -0.5f, 1, 0, 0},
        V{0.5f, 0.5f, 0.5f, 1, 0, 0},
        V{0.5f, -0.5f, -0.5f, 1, 0, 0},
        V{0.5f, 0.5f, 0.5f, 1, 0, 0},
        V{0.5f, -0.5f, 0.5f, 1, 0, 0},
        // -X
        V{-0.5f, -0.5f, -0.5f, -1, 0, 0},
        V{-0.5f, 0.5f, 0.5f, -1, 0, 0},
        V{-0.5f, 0.5f, -0.5f, -1, 0, 0},
        V{-0.5f, -0.5f, -0.5f, -1, 0, 0},
        V{-0.5f, -0.5f, 0.5f, -1, 0, 0},
        V{-0.5f, 0.5f, 0.5f, -1, 0, 0},
        // +Y
        V{-0.5f, 0.5f, -0.5f, 0, 1, 0},
        V{0.5f, 0.5f, 0.5f, 0, 1, 0},
        V{0.5f, 0.5f, -0.5f, 0, 1, 0},
        V{-0.5f, 0.5f, -0.5f, 0, 1, 0},
        V{-0.5f, 0.5f, 0.5f, 0, 1, 0},
        V{0.5f, 0.5f, 0.5f, 0, 1, 0},
        // -Y
        V{-0.5f, -0.5f, -0.5f, 0, -1, 0},
        V{0.5f, -0.5f, -0.5f, 0, -1, 0},
        V{0.5f, -0.5f, 0.5f, 0, -1, 0},
        V{-0.5f, -0.5f, -0.5f, 0, -1, 0},
        V{0.5f, -0.5f, 0.5f, 0, -1, 0},
        V{-0.5f, -0.5f, 0.5f, 0, -1, 0},
    };

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    mesh.vao.bind();
    mesh.vbo.bind();
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void *)(3 * sizeof(f32)));

    VBO::unbind();
    VAO::unbind();
    return mesh;
}

static GLMesh create_grid_mesh(const RenderSettings::GridSettings &gs) {
    struct V {
        f32 px, py, pz;
        f32 r, g, b, a;
    };

    const int N = std::max(1, gs.half_extent_lines);
    const f32 E = static_cast<f32>(N) * gs.spacing;

    std::vector<V> verts;
    verts.reserve(static_cast<usize>((2 * N + 1) * 4));

    auto push_line = [&](glm::vec3 a, glm::vec3 b, f32 r, f32 g, f32 bl, f32 al) {
        verts.push_back(V{a.x, a.y, a.z, r, g, bl, al});
        verts.push_back(V{b.x, b.y, b.z, r, g, bl, al});
    };

    // Lines parallel to Y axis (x = const)
    for (int i = -N; i <= N; ++i) {
        f32 x = static_cast<f32>(i) * gs.spacing;
        if (i == 0) {
            // y-axis: x=0 -> green
            push_line({x, -E, 0}, {x, E, 0}, 0.15f, 0.90f, 0.25f, gs.axis_alpha);
        } else {
            push_line({x, -E, 0}, {x, E, 0}, 0.65f, 0.68f, 0.72f, gs.minor_alpha);
        }
    }

    // Lines parallel to X axis (y = const)
    for (int i = -N; i <= N; ++i) {
        f32 y = static_cast<f32>(i) * gs.spacing;
        if (i == 0) {
            // x-axis: y=0 -> red
            push_line({-E, y, 0}, {E, y, 0}, 0.90f, 0.20f, 0.18f, gs.axis_alpha);
        } else {
            push_line({-E, y, 0}, {E, y, 0}, 0.65f, 0.68f, 0.72f, gs.minor_alpha);
        }
    }

    GLMesh mesh{};
    glGenVertexArrays(1, mesh.vao.ptr());
    glGenBuffers(1, mesh.vbo.ptr());
    mesh.vertex_count = static_cast<GLsizei>(verts.size());

    mesh.vao.bind();
    mesh.vbo.bind();
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(V)), verts.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(V), (void *)(3 * sizeof(f32)));

    VBO::unbind();
    VAO::unbind();
    return mesh;
}

// ---------------- GLFW Setup ----------------

enum class SetupGLFWError {
    InitFailed,
    WindowCreateFailed,
    GladInitFailed
};

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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return std::unexpected(SetupGLFWError::GladInitFailed);
    }

    return {window};
}

static void render_imgui_windows(OrbitCamera &cam, std::vector<Object> &objects, std::optional<usize> &selected_index) {
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
        ImGui::Text("  distance: %.3f", cam.distance);
        ImGui::Text("  yaw(deg):  %.2f", glm::degrees(cam.yaw));
        ImGui::Text("  pitch(deg):%.2f", glm::degrees(cam.pitch));
        ImGui::Text("  pivot:     (%.2f, %.2f, %.2f)", cam.pivot.x, cam.pivot.y, cam.pivot.z);
        ImGui::End();
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
        if (selected_index.has_value() && *selected_index < objects.size()) {
            Object &o = objects[*selected_index];
            ImGui::Text("Selected: %s", o.name.c_str());
            ImGui::Separator();

            ImGui::ColorEdit3("Color", &o.color.x);
            ImGui::DragFloat3("Position", &o.transform.position.x, 0.01f);
            ImGui::DragFloat3("Rotation (deg)", &o.transform.rotation_deg.x, 0.25f);
            ImGui::DragFloat3("Scale", &o.transform.scale.x, 0.01f, 0.001f, 1000.0f);

            o.transform.scale.x = std::max(o.transform.scale.x, 0.001f);
            o.transform.scale.y = std::max(o.transform.scale.y, 0.001f);
            o.transform.scale.z = std::max(o.transform.scale.z, 0.001f);

            if (ImGui::Button("Deselect")) {
                selected_index = std::nullopt;
            }
        } else {
            ImGui::Text("No object selected.");
            ImGui::Text("Left-click objects to select.");
            ImGui::Text("Middle-mouse drag to orbit.");
        }
        ImGui::End();
    }
}

static bool intersect_unit_cube_obb(
    const Ray &ray_world,
    const glm::mat4 &model,
    f32 &t_world_out) {
    const glm::mat4 invM = glm::inverse(model);

    const glm::vec3 oL = glm::vec3(invM * glm::vec4(ray_world.origin, 1.0f));
    const glm::vec3 dL = glm::vec3(invM * glm::vec4(ray_world.dir, 0.0f)); // keep paramization

    const glm::vec3 bmin(-0.5f), bmax(0.5f);

    f32 tmin = -1e30f;
    f32 tmax = 1e30f;

    auto slab = [&](f32 o, f32 d, f32 mn, f32 mx) -> bool {
        if (std::abs(d) < 1e-8f) {
            return (o >= mn && o <= mx);
        }
        f32 t1 = (mn - o) / d;
        f32 t2 = (mx - o) / d;
        if (t1 > t2)
            std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        return tmin <= tmax;
    };

    if (!slab(oL.x, dL.x, bmin.x, bmax.x))
        return false;
    if (!slab(oL.y, dL.y, bmin.y, bmax.y))
        return false;
    if (!slab(oL.z, dL.z, bmin.z, bmax.z))
        return false;

    // pick nearest positive local t
    f32 tL = (tmin > 0.0f) ? tmin : ((tmax > 0.0f) ? tmax : -1.0f);
    if (tL <= 0.0f)
        return false;

    const glm::vec3 hitL = oL + tL * dL;
    const glm::vec3 hitW = glm::vec3(model * glm::vec4(hitL, 1.0f));

    // Convert to world-ray parameter (dir is normalized)
    const f32 tW = glm::dot(hitW - ray_world.origin, ray_world.dir);
    if (tW <= 0.0f)
        return false;

    t_world_out = tW;
    return true;
}

} // namespace ds_pba

int main() {
    using namespace ds_pba;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    auto window_res = setup_glfw();
    if (!window_res.has_value()) {
        std::println(stderr, "Failed to setup glfw with error code: {}", static_cast<int>(window_res.error()));
        return EXIT_FAILURE;
    }
    GLFWwindow *window = *window_res;

    // Use 330 for layout(location=...) and modern GLSL
    const char *glsl_version = "#version 330";

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::println(stderr, "ImGui_ImplGlfw_InitForOpenGL failed");
        return EXIT_FAILURE;
    }
    if (!ImGui_ImplOpenGL3_Init(glsl_version)) {
        std::println(stderr, "ImGui_ImplOpenGL3_Init failed");
        return EXIT_FAILURE;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    const char *grid_vs = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;

uniform mat4 uView;
uniform mat4 uProj;

out vec4 vColor;
out vec3 vWorldPos;

void main() {
    vColor = aColor;
    vWorldPos = aPos; // grid in world space
    gl_Position = uProj * uView * vec4(aPos, 1.0);
}
)";

    const char *grid_fs = R"(#version 330 core
in vec4 vColor;
in vec3 vWorldPos;

uniform float uFogStart;
uniform float uFogEnd;

out vec4 FragColor;

void main() {
    float d = length(vWorldPos.xy);
    float t = clamp((d - uFogStart) / max(1e-6, (uFogEnd - uFogStart)), 0.0, 1.0);
    float fog = 1.0 - t;

    float alpha = vColor.a * fog;
    FragColor = vec4(vColor.rgb, alpha);
}
)";

    const char *obj_vs = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;

    mat3 normalMat = transpose(inverse(mat3(uModel)));
    vNormal = normalize(normalMat * aNormal);

    gl_Position = uProj * uView * world;
}
)";

    const char *obj_fs = R"(#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uColor;
uniform vec3 uCameraPos;
uniform float uTime;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vec3(0.35, 0.55, 0.75));
    float diff = max(dot(N, L), 0.0);

    vec3 base = uColor * (0.20 + 0.80 * diff);

    FragColor = vec4(base, 1.0);
}
)";
    const char *outline_vs = R"(#version 330 core
layout(location=0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
)";

    const char *outline_fs = R"(#version 330 core
uniform vec3 uColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(uColor, 1.0);
}
)";
    ShaderProgram grid_prog = create_program(grid_vs, grid_fs);
    ShaderProgram obj_prog = create_program(obj_vs, obj_fs);
    ShaderProgram outline_prog = create_program(outline_vs, outline_fs);

    if (!grid_prog.valid() || !obj_prog.valid() || !outline_prog.valid()) {
        std::println(stderr, "Failed to create shader programs");
        return EXIT_FAILURE;
    }

    GLMesh cube_mesh = create_cube_mesh();
    GLMesh grid_mesh = create_grid_mesh(g_render_settings.grid);

    std::vector<Object> objects;
    objects.push_back(Object{
        .name = "Cube A",
        .transform = Transform{.position = {2.0f, 1.0f, 0.5f}, .rotation_deg = {0, 0, 0}, .scale = {1, 1, 1}},
        .color = {0.85f, 0.35f, 0.25f},
    });
    objects.push_back(Object{
        .name = "Cube B",
        .transform = Transform{.position = {-1.5f, 2.5f, 0.5f}, .rotation_deg = {0, 0, 25}, .scale = {1, 1, 1}},
        .color = {0.25f, 0.55f, 0.90f},
    });
    objects.push_back(Object{
        .name = "Cube C",
        .transform = Transform{.position = {-2.5f, -1.5f, 0.75f}, .rotation_deg = {15, 0, 0}, .scale = {1.5f, 1.0f, 1.5f}},
        .color = {0.30f, 0.85f, 0.45f},
    });

    std::optional<usize> selected_index = std::nullopt;

    OrbitCamera camera{};
    camera.pivot = {0, 0, 0};
    camera.distance = 10.0f;

    auto framebuffer_callback = [](GLFWwindow *, int width, int height) -> void {
        glViewport(0, 0, width, height);
    };
    glfwSetFramebufferSizeCallback(window, framebuffer_callback);

    {
        int fbw = 0, fbh = 0;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
    }

    bool prev_left = false;
    bool prev_middle = false;
    f64 prev_mx = 0.0;
    f64 prev_my = 0.0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiIO &io = ImGui::GetIO();
        const bool imgui_wants_mouse = io.WantCaptureMouse;

        { // Handle Inputs
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        }

        f64 mouse_x = 0.0;
        f64 mouse_y = 0.0;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);

        const bool left_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool middle_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;

        if (!imgui_wants_mouse) {
            const f32 wheel = io.MouseWheel;
            if (wheel != 0.0f) {
                constexpr f32 zoom_speed = 0.12f; // tweak
                camera.distance *= std::exp(-wheel * zoom_speed);
                camera.distance = std::clamp(camera.distance, 0.75f, 200.0f);
            }
        }
        if (middle_down && !imgui_wants_mouse) {
            if (prev_middle) {
                const f32 dx = static_cast<f32>(mouse_x - prev_mx);
                const f32 dy = static_cast<f32>(mouse_y - prev_my);

                constexpr f32 sensitivity = 0.0050f;
                camera.yaw += -dx * sensitivity;
                camera.pitch += dy * sensitivity;

                const f32 lim = glm::radians(89.0f);
                camera.pitch = std::clamp(camera.pitch, -lim, lim);
            }
        }

        if (left_down && !prev_left && !imgui_wants_mouse) {
            int fbw = 1, fbh = 1;
            glfwGetFramebufferSize(window, &fbw, &fbh);
            const f32 aspect = (fbh > 0) ? (static_cast<f32>(fbw) / static_cast<f32>(fbh)) : 1.0f;

            const glm::mat4 V = camera.view_matrix();
            const glm::mat4 P = camera.proj_matrix(aspect);

            const Ray ray = ray_from_mouse(window, mouse_x, mouse_y, V, P);

            std::optional<usize> best_idx{};
            f32 best_t = 1e30f;

            for (usize i = 0; i < objects.size(); ++i) {
                const Object &o = objects[i];
                const glm::mat4 M = o.transform.model_matrix();

                f32 tW = 0.0f;
                if (intersect_unit_cube_obb(ray, M, tW)) {
                    if (tW < best_t) {
                        best_t = tW;
                        best_idx = i;
                    }
                }
            }

            selected_index = best_idx;
        }

        prev_left = left_down;
        prev_middle = middle_down;
        prev_mx = mouse_x;
        prev_my = mouse_y;

        render_imgui_windows(camera, objects, selected_index);
        ImGui::Render();

        int fbw = 1, fbh = 1;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        const f32 aspect = (fbh > 0) ? (static_cast<f32>(fbw) / static_cast<f32>(fbh)) : 1.0f;

        const glm::mat4 V = camera.view_matrix();
        const glm::mat4 P = camera.proj_matrix(aspect);

        auto bg = g_render_settings.background_color;
        glClearColor(bg.r(), bg.g(), bg.b(), bg.a());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        { // Draw Grid
            glDepthMask(GL_FALSE);
            grid_prog.bind();

            set_uniform_mat4(grid_prog.id, "uView", V);
            set_uniform_mat4(grid_prog.id, "uProj", P);
            set_uniform_float(grid_prog.id, "uFogStart", g_render_settings.grid.fog_start);
            set_uniform_float(grid_prog.id, "uFogEnd", g_render_settings.grid.fog_end);
            set_uniform_float(grid_prog.id, "uMinorAlpha", g_render_settings.grid.minor_alpha);
            set_uniform_float(grid_prog.id, "uAxisAlpha", g_render_settings.grid.axis_alpha);

            grid_mesh.vao.bind();
            glDrawArrays(GL_LINES, 0, grid_mesh.vertex_count);
            VAO::unbind();

            glDepthMask(GL_TRUE);
        }

        {
            obj_prog.bind();
            set_uniform_mat4(obj_prog.id, "uView", V);
            set_uniform_mat4(obj_prog.id, "uProj", P);
            set_uniform_vec3(obj_prog.id, "uCameraPos", camera.position());
            set_uniform_float(obj_prog.id, "uTime", static_cast<float>(glfwGetTime()));

            cube_mesh.vao.bind();
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);

            for (usize i = 0; i < objects.size(); ++i) {
                const Object &o = objects[i];
                const glm::mat4 M = o.transform.model_matrix();

                set_uniform_mat4(obj_prog.id, "uModel", M);
                set_uniform_vec3(obj_prog.id, "uColor", o.color);

                bool is_selected = selected_index.has_value() && (*selected_index == i);
                if (is_selected) {
                    glStencilFunc(GL_ALWAYS, 1, 0xFF);
                    glStencilMask(0xFF);
                } else {
                    glStencilMask(0x00);
                }
                set_uniform_bool(obj_prog.id, "uSelected", is_selected);

                glDrawArrays(GL_TRIANGLES, 0, cube_mesh.vertex_count);
            }
            if (selected_index.has_value()) {
                const Object &o = objects[*selected_index];

                glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
                glStencilMask(0x00);
                glDisable(GL_DEPTH_TEST);

                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDisable(GL_CULL_FACE);
                glLineWidth(2.0f);

                outline_prog.bind();

                glm::mat4 M = o.transform.model_matrix();
                const f32 outline_scale = 1.04f;
                M = glm::scale(M, glm::vec3(outline_scale));

                set_uniform_mat4(outline_prog.id, "uModel", M);
                set_uniform_mat4(outline_prog.id, "uView", V);
                set_uniform_mat4(outline_prog.id, "uProj", P);
                glm::vec3 outline_color{1.0f, 0.55f, 0.0f};
                set_uniform_vec3(
                    outline_prog.id,
                    "uColor",
                    outline_color);

                cube_mesh.vao.bind();
                glDrawArrays(GL_TRIANGLES, 0, cube_mesh.vertex_count);
                VAO::unbind();

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glEnable(GL_DEPTH_TEST);
                glStencilMask(0xFF);
                glStencilFunc(GL_ALWAYS, 0, 0xFF);
            }

            VAO::unbind();
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    glDeleteProgram(grid_prog.id);
    glDeleteProgram(obj_prog.id);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}