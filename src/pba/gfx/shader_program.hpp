// pba/gfx/shader_program.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/geometry.hpp"
#include "pba/gfx/gl_types.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ds_pba
{

using UniformLocation = GLint;
inline constexpr UniformLocation k_uniform_not_set{-1};

struct ShaderProgram
{
    GLuint id{0};

    struct Uniforms
    {
        UniformLocation uView{k_uniform_not_set};
        UniformLocation uProj{k_uniform_not_set};

        UniformLocation uModel{k_uniform_not_set};

        UniformLocation uColor{k_uniform_not_set};

        UniformLocation uFogStart{k_uniform_not_set};
        UniformLocation uFogEnd{k_uniform_not_set};

        UniformLocation uDiffuseTex{k_uniform_not_set};
        UniformLocation uNormalTex{k_uniform_not_set};
        UniformLocation uEnvironmentTex{k_uniform_not_set};
        UniformLocation uCameraPos{k_uniform_not_set};
        UniformLocation uEnvLightStrength{k_uniform_not_set};

        auto reset() noexcept -> void
        {
            uView = k_uniform_not_set;
            uProj = k_uniform_not_set;
            uModel = k_uniform_not_set;
            uColor = k_uniform_not_set;
            uFogStart = k_uniform_not_set;
            uFogEnd = k_uniform_not_set;
            uDiffuseTex = k_uniform_not_set;
            uNormalTex = k_uniform_not_set;
            uEnvironmentTex = k_uniform_not_set;
            uCameraPos = k_uniform_not_set;
            uEnvLightStrength = k_uniform_not_set;
        }
    };

    Uniforms u{};

    ShaderProgram() = default;
    explicit ShaderProgram(GLuint program_id) noexcept : id{program_id}
    {
    }

    [[nodiscard]] auto valid() const noexcept -> bool
    {
        return id != 0;
    }
    [[nodiscard]] auto handle() const noexcept -> ProgramHandle
    {
        return ProgramHandle{id};
    }

    auto bind() const noexcept -> void
    {
        Expects(valid());
        glUseProgram(id);
    }

    static auto unbind() noexcept -> void
    {
        glUseProgram(0);
    }

    auto destroy() noexcept -> void
    {
        if (id == 0)
        {
            return;
        }
        glDeleteProgram(id);
        id = 0;
        u.reset();
    }

    auto init_uniform_locations() noexcept -> void
    {
        {
            Expects(valid());
        }
        u.reset();

        u.uView = glGetUniformLocation(id, "uView");
        u.uProj = glGetUniformLocation(id, "uProj");
        u.uModel = glGetUniformLocation(id, "uModel");
        u.uColor = glGetUniformLocation(id, "uColor");
        u.uFogStart = glGetUniformLocation(id, "uFogStart");
        u.uFogEnd = glGetUniformLocation(id, "uFogEnd");
        u.uDiffuseTex = glGetUniformLocation(id, "uDiffuseTex");
        u.uNormalTex = glGetUniformLocation(id, "uNormalTex");
        u.uEnvironmentTex = glGetUniformLocation(id, "uEnvironmentTex");
        u.uCameraPos = glGetUniformLocation(id, "uCameraPos");
        u.uEnvLightStrength = glGetUniformLocation(id, "uEnvLightStrength");
    }

    auto set_uView(const ViewMatrix& view_matrix) const noexcept -> void
    {
        {
            Expects(u.uView != k_uniform_not_set);
        }
        if (u.uView == k_uniform_not_set)
        {
            return;
        }
        glUniformMatrix4fv(u.uView, 1, GL_FALSE, glm::value_ptr(view_matrix.m));
    }

    auto set_uProj(const ProjMatrix& proj_matrix) const noexcept -> void
    {
        {
            Expects(u.uProj != k_uniform_not_set);
        }
        if (u.uProj == k_uniform_not_set)
        {
            return;
        }
        glUniformMatrix4fv(u.uProj, 1, GL_FALSE, glm::value_ptr(proj_matrix.m));
    }

    auto set_uModel(const ModelMatrix& model_matrix) const noexcept -> void
    {
        {
            Expects(u.uModel != k_uniform_not_set);
        }
        if (u.uModel == k_uniform_not_set)
        {
            return;
        }
        glUniformMatrix4fv(u.uModel, 1, GL_FALSE, glm::value_ptr(model_matrix.m));
    }

    auto set_uColor(const Color3& c) const noexcept -> void
    {
        {
            Expects(u.uColor != k_uniform_not_set);
        }
        if (u.uColor == k_uniform_not_set)
        {
            return;
        }
        glUniform3f(u.uColor, c.r(), c.g(), c.b());
    }

    auto set_uFogStart(f32 v) const noexcept -> void
    {
        {
            Expects(u.uFogStart != k_uniform_not_set);
        }
        if (u.uFogStart == k_uniform_not_set)
        {
            return;
        }
        glUniform1f(u.uFogStart, v);
    }

    auto set_uFogEnd(f32 v) const noexcept -> void
    {
        {
            Expects(u.uFogEnd != k_uniform_not_set);
        }
        if (u.uFogEnd == k_uniform_not_set)
        {
            return;
        }
        glUniform1f(u.uFogEnd, v);
    }

    auto set_uDiffuseTex(int unit) const noexcept -> void
    {
        {
            Expects(u.uDiffuseTex != k_uniform_not_set);
        }
        if (u.uDiffuseTex == k_uniform_not_set)
        {
            return;
        }
        glUniform1i(u.uDiffuseTex, unit);
    }

    auto set_uNormalTex(int unit) const noexcept -> void
    {
        {
            Expects(u.uNormalTex != k_uniform_not_set);
        }
        if (u.uNormalTex == k_uniform_not_set)
        {
            return;
        }
        glUniform1i(u.uNormalTex, unit);
    }

    auto set_uEnvironmentTex(int unit) const noexcept -> void
    {
        {
            Expects(u.uEnvironmentTex != k_uniform_not_set);
        }
        if (u.uEnvironmentTex == k_uniform_not_set)
        {
            return;
        }
        glUniform1i(u.uEnvironmentTex, unit);
    }

    auto set_uCameraPos(const Pos3& p) const noexcept -> void
    {
        {
            Expects(u.uCameraPos != k_uniform_not_set);
        }
        if (u.uCameraPos == k_uniform_not_set)
        {
            return;
        }
        glUniform3f(u.uCameraPos, p.x, p.y, p.z);
    }

    auto set_uEnvLightStrength(f32 v) const noexcept -> void
    {
        {
            Expects(u.uEnvLightStrength != k_uniform_not_set);
        }
        if (u.uEnvLightStrength == k_uniform_not_set)
        {
            return;
        }
        glUniform1f(u.uEnvLightStrength, v);
    }
};

}  // namespace ds_pba
