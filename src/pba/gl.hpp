// pba/gl.hpp
#pragma once

#include "types.hpp"

#include <expected>
#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ds_pba {


enum class ShaderLoadError {
    FileNotFound,
    IOError,
    EmptyFile
};

GLuint compile_shader(GLenum type, const char *src);

ShaderProgram create_program(
    const std::string &vert_src,
    const std::string &frag_src);

std::expected<ShaderProgram, ShaderLoadError>
create_program_from_file(std::string shader_name);

std::expected<std::string, ShaderLoadError>
read_text_file(const std::string &path);

std::expected<std::string, ShaderLoadError>
load_shader_sources(const std::string &shader_name);

inline void set_uniform_mat4(GLuint program, const char *name, const glm::mat4 &m) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
    }
}

inline void set_uniform_vec3(GLuint program, const char *name, const glm::vec3 &v) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) {
        glUniform3f(loc, v.x, v.y, v.z);
    }
}

inline void set_uniform_float(GLuint program, const char *name, f32 v) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) {
        glUniform1f(loc, v);
    }
}

inline void set_uniform_bool(GLuint program, const char *name, bool v) {
    GLint loc = glGetUniformLocation(program, name);
    if (loc >= 0) {
        glUniform1i(loc, v ? 1 : 0);
    }
}

} // namespace ds_pba