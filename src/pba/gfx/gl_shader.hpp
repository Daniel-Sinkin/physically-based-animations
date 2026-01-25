// pba/gfx/gl_shader.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/gfx/gl_types.hpp"
//
#include <algorithm>
#include <cassert>
#include <glad/glad.h>
#include <string>

namespace ds_pba
{
enum class ShaderType : GLenum
{
    Vertex = GL_VERTEX_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
    Geometry = GL_GEOMETRY_SHADER,
    TessCtrl = GL_TESS_CONTROL_SHADER,
    TessEval = GL_TESS_EVALUATION_SHADER
};

struct Shader
{
    ShaderHandle handle{};
    ShaderType type{};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return handle.valid();
    }

    [[nodiscard]] static constexpr bool is_valid_type(ShaderType type) noexcept
    {
        switch (type)
        {
            case ShaderType::Vertex:
            case ShaderType::Fragment:
            case ShaderType::Geometry:
            case ShaderType::TessCtrl:
            case ShaderType::TessEval:
                return true;
            default:
                return false;
        }
    }

    static Shader create(ShaderType shader_type) noexcept
    {
        assert(is_valid_type(shader_type) && "Invalid ShaderType");
        return Shader{
            .handle = ShaderHandle{glCreateShader(static_cast<GLenum>(shader_type))},
            .type = shader_type
        };
    }

    void compile(const std::string& source) const noexcept
    {
        assert(valid() && "Attempting to compile invalid Shader (id == 0)");
        const char* src{source.data()};
        const GLint len{static_cast<GLint>(source.size())};
        glShaderSource(handle.id, 1, &src, &len);
        glCompileShader(handle.id);
    }

    static Shader create_and_compile(ShaderType shader_type, const std::string& source) noexcept
    {
        Shader shader{Shader::create(shader_type)};
        shader.compile(source);
        return shader;
    }

    [[nodiscard]] bool compiled_ok() const noexcept
    {
        assert(valid() && "Attempting to query invalid Shader (id == 0)");
        GLint ok{GL_FALSE};
        glGetShaderiv(handle.id, GL_COMPILE_STATUS, &ok);
        return ok == GL_TRUE;
    }

    [[nodiscard]] std::string info_log() const
    {
        assert(valid() && "Attempting to query invalid Shader (id == 0)");
        GLint log_len{0};
        glGetShaderiv(handle.id, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<usize>(std::max(1, log_len)), '\0');
        glGetShaderInfoLog(handle.id, log_len, nullptr, log.data());
        return log;
    }

    void destroy() noexcept
    {
        if (valid())
        {
            glDeleteShader(handle.id);
            handle.id = 0;
        }
    }
};

}  // namespace ds_pba
