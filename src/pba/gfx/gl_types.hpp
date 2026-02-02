// pba/render/gl_types.hpp
#pragma once

#include "pba/core/core_types.hpp"
//
#include <cassert>
#include <cstddef>
//
#include <glad/glad.h>
#include <gsl/assert>

namespace ds_pba
{

using UniformLocation = GLint;

// OpenGL uses void* offsets, helper to avoid having this casting workaround everywhere
struct GLPtr final
{
    static constexpr auto offset0() noexcept -> const void*
    {
        return static_cast<const void*>(nullptr);
    }

    static constexpr const void* offset(usize bytes) noexcept
    {
        return reinterpret_cast<const void*>(static_cast<uptr>(bytes));
    }
};

struct ShaderHandle final
{
    GLuint id{0};

    constexpr ShaderHandle() = default;
    constexpr explicit ShaderHandle(GLuint v) noexcept : id{v}
    {
    }

    [[nodiscard]] constexpr auto valid() const noexcept -> bool
    {
        return id != 0;
    }

    friend constexpr auto operator==(ShaderHandle a, ShaderHandle b) noexcept -> bool
    {
        return a.id == b.id;
    }
};

struct VAO final
{
    GLuint id{0};

    constexpr auto ptr() noexcept -> GLuint*
    {
        return &id;
    }

    [[nodiscard]] constexpr auto valid() const noexcept -> bool
    {
        return id != 0;
    }

    auto destroy() noexcept -> void
    {
        if (valid())
        {
            glDeleteVertexArrays(1, &id);
            id = 0;
        }
    }

    auto bind() const noexcept -> void
    {
        {
            Expects(valid() && "Attempting to bind invalid VAO (id == 0)");
        }
        glBindVertexArray(id);
    }

    static auto bind(GLuint v) noexcept -> void
    {
        glBindVertexArray(v);
    }

    [[nodiscard]] static auto current_binding() noexcept -> GLuint
    {
        GLint v{0};
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &v);
        return static_cast<GLuint>(v);
    }

    static auto unbind() noexcept -> void
    {
        glBindVertexArray(0);
    }
};

struct VBO final
{
    GLuint id{0};

    constexpr auto ptr() noexcept -> GLuint*
    {
        return &id;
    }

    [[nodiscard]] constexpr auto valid() const noexcept -> bool
    {
        return id != 0;
    }

    auto destroy() noexcept -> void
    {
        if (valid())
        {
            glDeleteBuffers(1, &id);
            id = 0;
        }
    }

    auto bind_array_buffer() const noexcept -> void
    {
        {
            Expects(valid() && "Attempting to bind invalid VBO (id == 0)");
        }
        glBindBuffer(GL_ARRAY_BUFFER, id);
    }

    static auto bind_array_buffer(GLuint v) noexcept -> void
    {
        glBindBuffer(GL_ARRAY_BUFFER, v);
    }

    [[nodiscard]] static auto current_array_buffer_binding() noexcept -> GLuint
    {
        GLint v{0};
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &v);
        return static_cast<GLuint>(v);
    }

    static void unbind() noexcept
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

struct ProgramHandle final
{
    GLuint id{0};

    constexpr ProgramHandle() = default;
    constexpr explicit ProgramHandle(GLuint v) noexcept : id{v}
    {
    }

    [[nodiscard]] constexpr auto valid() const noexcept -> bool
    {
        return id != 0;
    }

    friend constexpr auto operator==(ProgramHandle a, ProgramHandle b) noexcept -> bool
    {
        return a.id == b.id;
    }
};

struct GLMesh final
{
    VAO vao{};
    VBO vbo{};
    GLsizei vertex_count{0};

    auto instantiate_once() const noexcept -> void
    {
        const GLuint prev{VAO::current_binding()};
        vao.bind();
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
        VAO::bind(prev);
    }

    auto destroy() noexcept -> void
    {
        vbo.destroy();
        vao.destroy();
        vertex_count = 0;
    }
};

class ScopedBufferBinder final
{
    // The 'current_binding()' functions of VAO and VBO are expensive, don't use this in a hot path.
  public:
    explicit ScopedBufferBinder(const GLMesh& mesh) noexcept
        : prev_vao_{VAO::current_binding()}, prev_vbo_{VBO::current_array_buffer_binding()}
    {
        mesh.vao.bind();
        mesh.vbo.bind_array_buffer();
    }

    ~ScopedBufferBinder() noexcept
    {
        VAO::bind(prev_vao_);
        VBO::bind_array_buffer(prev_vbo_);
    }

    ScopedBufferBinder(const ScopedBufferBinder&) = delete;
    ScopedBufferBinder& operator=(const ScopedBufferBinder&) = delete;
    ScopedBufferBinder(ScopedBufferBinder&&) = delete;
    ScopedBufferBinder& operator=(ScopedBufferBinder&&) = delete;

  private:
    GLuint prev_vao_{0};
    GLuint prev_vbo_{0};
};

}  // namespace ds_pba

namespace std
{
template <>
struct hash<ds_pba::ProgramHandle>
{
    auto operator()(ds_pba::ProgramHandle h) const noexcept -> ds_pba::usize
    {
        return std::hash<GLuint>{}(h.id);
    }
};
}  // namespace std
