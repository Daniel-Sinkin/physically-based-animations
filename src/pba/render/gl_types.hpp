// pba/gl_types.hpp
#pragma once

#include "pba/core_types.hpp"

#include <cassert>
#include <cstddef>
#include <glad/glad.h>

namespace ds_pba
{

using UniformLocation = GLint;

// OpenGL uses void* offsets, helper to avoid having this casting workaround everywhere
struct GLPtr final
{
    static constexpr const void* offset0() noexcept
    {
        return static_cast<const void*>(nullptr);
    }

    static constexpr const void* offset(usize bytes) noexcept
    {
        return reinterpret_cast<const void*>(static_cast<uptr>(bytes));
    }
};

struct ShaderHandle
{
    GLuint id{0};

    constexpr ShaderHandle() = default;
    constexpr explicit ShaderHandle(GLuint v) noexcept : id{v}
    {
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return id != 0;
    }

    friend constexpr bool operator==(ShaderHandle a, ShaderHandle b) noexcept
    {
        return a.id == b.id;
    }
};

struct VAO
{
    GLuint id{};

    constexpr GLuint* ptr() noexcept
    {
        return &id;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return id != 0;
    }

    void bind() const noexcept
    {
        assert(valid() && "Attempting to bind invalid VAO (id == 0)");
        glBindVertexArray(id);
    }

    static void unbind() noexcept
    {
        glBindVertexArray(0);
    }
};

struct VBO
{
    GLuint id{};

    constexpr GLuint* ptr() noexcept
    {
        return &id;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return id != 0;
    }

    void bind_array_buffer() const noexcept
    {
        assert(valid() && "Attempting to bind invalid VBO (id == 0)");
        glBindBuffer(GL_ARRAY_BUFFER, id);
    }

    static void unbind() noexcept
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

struct ProgramHandle
{  // This is probably completely overkill regarding strong typing
    GLuint id{0};

    constexpr ProgramHandle() = default;
    constexpr explicit ProgramHandle(GLuint v) noexcept : id{v}
    {
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return id != 0;
    }

    friend constexpr bool operator==(ProgramHandle a, ProgramHandle b) noexcept
    {
        return a.id == b.id;
    }
};
struct ShaderProgram
{
    GLuint id{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return id != 0;
    }

    [[nodiscard]] constexpr ProgramHandle handle() const noexcept
    {
        return ProgramHandle{id};
    }

    void bind() const noexcept
    {
        assert(valid());
        glUseProgram(id);
    }

    static void unbind() noexcept
    {
        glUseProgram(0);
    }
};

struct GLMesh
{
    VAO vao{};
    VBO vbo{};
    GLsizei vertex_count{};

    void instantiate_once() const noexcept
    {
        vao.bind();
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
        VAO::unbind();
    }
};

class ScopedBufferBinder
{
  public:
    ScopedBufferBinder(GLMesh& mesh)
    {
        mesh.vao.bind();
        mesh.vbo.bind_array_buffer();
    }
    ~ScopedBufferBinder()
    {
        VBO::unbind();
        VAO::unbind();
    }

    ScopedBufferBinder(const ScopedBufferBinder&) = delete;
    ScopedBufferBinder& operator=(const ScopedBufferBinder&) = delete;

    ScopedBufferBinder(ScopedBufferBinder&&) = delete;
    ScopedBufferBinder& operator=(ScopedBufferBinder&&) = delete;
};

}  // namespace ds_pba

namespace std
{
template <>
struct hash<ds_pba::ProgramHandle>
{
    size_t operator()(ds_pba::ProgramHandle h) const noexcept
    {
        return std::hash<GLuint>{}(h.id);
    }
};
}  // namespace std
