// pba/gl.cpp
#include "gl.hpp"

#include <algorithm>
#include <fstream>
#include <print>
#include <sstream>

namespace ds_pba {

GLuint compile_shader(GLenum type, const char *src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint log_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<usize>(std::max(1, log_len)), '\0');
        glGetShaderInfoLog(shader, log_len, nullptr, log.data());
        std::println(stderr, "Shader compile failed:\n{}", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}


ShaderProgram create_program(
    const std::string &vert_src,
    const std::string &frag_src) {

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vert_src.c_str());
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag_src.c_str());

    if (vs == 0 || fs == 0) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return ShaderProgram{0};
    }

    ShaderProgram prog{glCreateProgram()};
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint log_len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<usize>(std::max(1, log_len)), '\0');
        glGetProgramInfoLog(prog, log_len, nullptr, log.data());
        std::println(stderr, "Program link failed:\n{}", log);
        glDeleteProgram(prog.id);
        prog.id = 0;
    }
    return prog;
}

std::expected<std::string, ShaderLoadError>
read_text_file(const std::string &path) {
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) {
        return std::unexpected(ShaderLoadError::FileNotFound);
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    if (file.fail() && !file.eof()) {
        return std::unexpected(ShaderLoadError::IOError);
    }

    std::string contents = ss.str();
    if (contents.empty()) {
        return std::unexpected(ShaderLoadError::EmptyFile);
    }

    return contents;
}

std::expected<std::string, ShaderLoadError>
load_shader_sources(const std::string &shader_name) {
    const std::string path = "assets/shaders/" + shader_name;
    auto shader = read_text_file(path);
    if (!shader) {
        return std::unexpected(shader.error());
    }
    return std::move(*shader);
}

std::expected<ShaderProgram, ShaderLoadError>
create_program_from_file(std::string shader_name) {
    auto frag = load_shader_sources(shader_name + ".frag");
    if (!frag) {
        return std::unexpected(frag.error());
    }

    auto vert = load_shader_sources(shader_name + ".vert");
    if (!vert) {
        return std::unexpected(vert.error());
    }

    return create_program(*vert, *frag);
}

} // namespace ds_pba