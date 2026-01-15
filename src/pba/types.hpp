// types.hpp
#pragma once

#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
        return std::abs(dir.length() - 1.0) < 0.0001;
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