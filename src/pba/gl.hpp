// pba/gl.hpp
#pragma once

#include "pba/core_types.hpp"
#include "pba/gl_types.hpp"

#include <cassert>
#include <expected>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <string>

namespace ds_pba
{
enum class ShaderCreateError
{
    FileNotFound,
    IOError,
    EmptyFile,
    CompilationError
};

std::optional<ShaderProgram>
create_program(const std::string& vert_src, const std::string& frag_src);

std::expected<ShaderProgram, ShaderCreateError> create_program_from_file(std::string shader_name);

std::expected<std::string, ShaderCreateError> read_text_file(const std::string& path);

std::expected<std::string, ShaderCreateError> load_shader_sources(const std::string& shader_name);

inline void set_uniform_mat4(GLuint program, const char* name, const glm::mat4& m)
{
    const GLint loc{glGetUniformLocation(program, name)};
    assert(loc >= 0);
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

inline void set_uniform_vec3(GLuint program, const char* name, const glm::vec3& v)
{
    const GLint loc{glGetUniformLocation(program, name)};
    assert(loc >= 0);
    glUniform3f(loc, v.x, v.y, v.z);
}

inline void set_uniform_float(GLuint program, const char* name, f32 v)
{
    const GLint loc{glGetUniformLocation(program, name)};
    assert(loc >= 0);
    glUniform1f(loc, v);
}

inline void set_uniform_bool(GLuint program, const char* name, bool v)
{
    const GLint loc{glGetUniformLocation(program, name)};
    assert(loc >= 0);
    glUniform1i(loc, v ? 1 : 0);
}

}  // namespace ds_pba
