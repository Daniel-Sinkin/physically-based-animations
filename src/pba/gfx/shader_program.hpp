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

        void reset() noexcept
        {
            uView = k_uniform_not_set;
            uProj = k_uniform_not_set;
            uModel = k_uniform_not_set;
            uColor = k_uniform_not_set;
            uFogStart = k_uniform_not_set;
            uFogEnd = k_uniform_not_set;
            uDiffuseTex = k_uniform_not_set;
            uNormalTex = k_uniform_not_set;
        }
    };

    Uniforms u{};

    ShaderProgram() = default;
    explicit ShaderProgram(GLuint program_id) noexcept : id{program_id}
    {
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return id != 0;
    }
    [[nodiscard]] ProgramHandle handle() const noexcept
    {
        return ProgramHandle{id};
    }

    void bind() const noexcept
    {
        Expects(valid());
        glUseProgram(id);
    }

    static void unbind() noexcept
    {
        glUseProgram(0);
    }

    void destroy() noexcept
    {
        if (id == 0)
        {
            return;
        }
        glDeleteProgram(id);
        id = 0;
        u.reset();
    }

    void init_uniform_locations() noexcept
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
    }

    void set_uView(const ViewMatrix& view_matrix) const noexcept
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

    void set_uProj(const ProjMatrix& proj_matrix) const noexcept
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

    void set_uModel(const ModelMatrix& model_matrix) const noexcept
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

    void set_uColor(const Color3& c) const noexcept
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

    void set_uFogStart(f32 v) const noexcept
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

    void set_uFogEnd(f32 v) const noexcept
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

    void set_uDiffuseTex(int unit) const noexcept
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

    void set_uNormalTex(int unit) const noexcept
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
};

}  // namespace ds_pba
