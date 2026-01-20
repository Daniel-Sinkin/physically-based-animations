// pba/gl_types.hpp
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <glad/glad.h>

namespace ds_pba {

// OpenGL uses void* offsets, helper to avoid having this casting workaround everywhere
struct GLPtr final {
    static constexpr const void *offset0() noexcept {
        return static_cast<const void *>(nullptr);
    }

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

struct GLMesh {
    VAO vao{};
    VBO vbo{};
    GLsizei vertex_count{};

    void instantiate_once() const noexcept {
        vao.bind();
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
        VAO::unbind();
    }
};

} // namespace ds_pba
