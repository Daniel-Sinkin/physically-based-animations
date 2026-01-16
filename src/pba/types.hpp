// pba/types.hpp
#pragma once

#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <utility>

#include "imgui.h"

namespace ds_pba {
using usize = std::size_t;

using i64 = std::int64_t;
using i32 = std::int32_t;
using i16 = std::int16_t;
using i8 = std::int8_t;

using u64 = std::uint64_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;

#if defined(__cpp_lib_stdfloat) && __cpp_lib_stdfloat >= 202207L
using f32 = std::float32_t;
using f64 = std::float64_t;
#else
using f32 = float;
using f64 = double;
#endif
static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

// OpenGL uses void* pointers this wrapper, this wrapper cleanly adjusts to this without
// triggering any conversion errors; see for example docs of glVertexAttribPointer
struct GLPtr final {
    // Offset 0 (start of buffer).
    static constexpr const void *offset0() noexcept {
        return static_cast<const void *>(nullptr);
    }

    // Offset 'bytes' into the currently bound buffer.
    static constexpr const void *offset(std::size_t bytes) noexcept {
        return reinterpret_cast<const void *>(static_cast<std::uintptr_t>(bytes));
    }
};

struct VAO {
    GLuint id{};

    constexpr operator GLuint() const noexcept { return id; }
    constexpr GLuint *ptr() noexcept { return &id; }

    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }

    void bind() const noexcept {
        assert(valid() && "Attempting to bind invalid VAO (id == 0)");
        glBindVertexArray(id);
    }

    static void unbind() noexcept { glBindVertexArray(0); }
};

struct VBO {
    GLuint id{};

    constexpr operator GLuint() const noexcept { return id; }
    constexpr GLuint *ptr() noexcept { return &id; }

    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }

    void bind() const noexcept {
        assert(valid() && "Attempting to bind invalid VBO (id == 0)");
        glBindBuffer(GL_ARRAY_BUFFER, id);
    }

    static void unbind() noexcept { glBindBuffer(GL_ARRAY_BUFFER, 0); }
};

struct ShaderProgram {
    GLuint id{};

    constexpr operator GLuint() const noexcept { return id; }

    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }

    void bind() const noexcept {
        assert(valid() && "Attempting to bind invalid ShaderProgram (id == 0)");
        glUseProgram(id);
    }

    static void unbind() noexcept { glUseProgram(0); }
};

enum class ShaderType : GLenum {
    Vertex = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
    Geometry = GL_GEOMETRY_SHADER,
    TessCtrl = GL_TESS_CONTROL_SHADER,
    TessEval = GL_TESS_EVALUATION_SHADER
};
struct Shader {
    GLuint id{};
    ShaderType type{};

    constexpr operator GLuint() const noexcept { return id; }
    constexpr GLuint *ptr() noexcept { return &id; }

    [[nodiscard]] constexpr bool valid() const noexcept { return id != 0; }

    [[nodiscard]] static constexpr bool is_valid_type(ShaderType t) noexcept {
        switch (t) {
        case ShaderType::Vertex:
        case ShaderType::Fragment:
        case ShaderType::Geometry:
        case ShaderType::TessCtrl:
        case ShaderType::TessEval:
            return true;
        default:
            return false;
        }
        std::unreachable();
    }

    static Shader create(ShaderType shader_type) noexcept {
        assert(is_valid_type(shader_type) && "Invalid ShaderType");
        Shader s{};
        s.type = shader_type;
        s.id = glCreateShader(static_cast<GLenum>(shader_type));
        return s;
    }

    void compile(const std::string &source) const noexcept {
        assert(valid() && "Attempting to compile invalid Shader (id == 0)");
        const char *src = source.data();
        const GLint len = static_cast<GLint>(source.size());
        glShaderSource(id, 1, &src, &len);
        glCompileShader(id);
    }

    static Shader create_and_compile(ShaderType shader_type, const std::string &source) noexcept {
        Shader shader = Shader::create(shader_type);
        shader.compile(source);
        return shader;
    }

    [[nodiscard]] bool compiled_ok() const noexcept {
        assert(valid() && "Attempting to query invalid Shader (id == 0)");
        GLint ok = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
        return ok == GL_TRUE;
    }

    [[nodiscard]] std::string info_log() const {
        assert(valid() && "Attempting to query invalid Shader (id == 0)");
        GLint log_len = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<usize>(std::max(1, log_len)), '\0');
        glGetShaderInfoLog(id, log_len, nullptr, log.data());
        return log;
    }

    void destroy() noexcept {
        if (id != 0) {
            glDeleteShader(id);
            id = 0;
        }
    }
};

struct GLMesh {
    VAO vao{};
    VBO vbo{};
    GLsizei vertex_count{};
};

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
    glm::vec3 dir{};

    bool valid() const { // Rays must have normalised length
        return std::abs(glm::length(dir) - 1.0f) < 0.0001f;
    }
};

struct ViewportFBO {
    GLuint fbo = 0;
    GLuint color_tex = 0;
    GLuint depth_rbo = 0;
    int width = 0;
    int height = 0;

    void destroy() noexcept {
        if (depth_rbo) {
            glDeleteRenderbuffers(1, &depth_rbo);
            depth_rbo = 0;
        }
        if (color_tex) {
            glDeleteTextures(1, &color_tex);
            color_tex = 0;
        }
        if (fbo) {
            glDeleteFramebuffers(1, &fbo);
            fbo = 0;
        }
        width = 0;
        height = 0;
    }

    bool ensure_size(int w, int h) noexcept {
        w = std::max(1, w);
        h = std::max(1, h);

        if (w == width && h == height && fbo && color_tex && depth_rbo) {
            return true;
        }

        destroy();

        width = w;
        height = h;

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        // Color texture
        glGenTextures(1, &color_tex);
        glBindTexture(GL_TEXTURE_2D, color_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // GL_RGBA8 is fine for UI viewport
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);

        // Depth+stencil renderbuffer
        glGenRenderbuffers(1, &depth_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);

        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        // Cleanup bindings
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return status == GL_FRAMEBUFFER_COMPLETE;
    }

    [[nodiscard]] ImTextureID imgui_texture_id() const noexcept {
        return static_cast<ImTextureID>(static_cast<std::uintptr_t>(color_tex));
    }
};

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = std::chrono::duration<f64>;

template <typename T>
struct Rect {
    T x, y, width, height;
};
using RectInt = Rect<int>;
using RectF32 = Rect<f32>;
using RectF64 = Rect<f64>;

template <typename T>
struct Vec4 {
    T x, y, z, w;
};

template <typename T>
struct ColorRGBA {
    std::array<T, 4> v;

    constexpr ColorRGBA() = default;
    constexpr ColorRGBA(T r, T g, T b, T a) noexcept : v{r, g, b, a} {}

    constexpr T *data() noexcept { return v.data(); }
    constexpr const T *data() const noexcept { return v.data(); }

    constexpr T &r() noexcept { return v[0]; }
    constexpr T &g() noexcept { return v[1]; }
    constexpr T &b() noexcept { return v[2]; }
    constexpr T &a() noexcept { return v[3]; }

    const T &r() const noexcept { return v[0]; }
    const T &g() const noexcept { return v[1]; }
    const T &b() const noexcept { return v[2]; }
    const T &a() const noexcept { return v[3]; }
};
using ColorRGBA8 = ColorRGBA<u8>;
using ColorRGBAf = ColorRGBA<f32>;
using ColorRGBAd = ColorRGBA<f64>;

} // namespace ds_pba