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

    [[nodiscard]] constexpr auto valid() const noexcept -> bool
    {
        return handle.valid();
    }

    [[nodiscard]] static constexpr auto valid(ShaderType type) noexcept -> bool
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

    static auto create(ShaderType shader_type) noexcept -> Shader
    {
        {
            Expects(valid(shader_type));
        }
        return Shader{
            .handle = ShaderHandle{glCreateShader(static_cast<GLenum>(shader_type))},
            .type = shader_type
        };
    }

    auto compile(const std::string& source) const noexcept -> void
    {
        {
            Expects(valid());
        }
        const char* src{source.data()};
        const auto len = static_cast<GLint>(source.size());
        glShaderSource(handle.id, 1, &src, &len);
        glCompileShader(handle.id);
    }

    static auto create_and_compile(ShaderType shader_type, const std::string& source) noexcept
        -> Shader
    {
        Shader shader{Shader::create(shader_type)};
        shader.compile(source);
        return shader;
    }

    [[nodiscard]] auto compiled_ok() const noexcept -> bool
    {
        {
            Expects(valid());
        }
        GLint ok{GL_FALSE};
        glGetShaderiv(handle.id, GL_COMPILE_STATUS, &ok);
        return ok == GL_TRUE;
    }

    [[nodiscard]] auto info_log() const -> std::string
    {
        {
            Expects(valid());
        }
        GLint log_len{0};
        glGetShaderiv(handle.id, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<usize>(std::max(1, log_len)), '\0');
        glGetShaderInfoLog(handle.id, log_len, nullptr, log.data());
        return log;
    }

    auto destroy() noexcept -> void
    {
        if (valid())
        {
            glDeleteShader(handle.id);
            handle.id = 0;
        }
    }
};

}  // namespace ds_pba
