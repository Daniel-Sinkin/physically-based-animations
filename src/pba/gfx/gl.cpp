// pba/gfx/gl.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gl.hpp"
//
#include "pba/gfx/gl_shader.hpp"
#include "pba/gfx/gl_types.hpp"
#include "pba/gfx/shader_program.hpp"

namespace ds_pba
{
auto create_program(const std::string& vert_src, const std::string& frag_src)
    -> std::optional<ShaderProgram>
{
    Shader vs{Shader::create_and_compile(ShaderType::Vertex, vert_src)};
    if (!vs.valid() || !vs.compiled_ok())
    {
        std::println(stderr, "Failed Vertex Shader Creation");
        return std::nullopt;
    }
    Shader fs{Shader::create_and_compile(ShaderType::Fragment, frag_src)};
    if (!fs.valid() || !fs.compiled_ok())
    {
        std::println(stderr, "Failed Fragment Shader Creation");
        return std::nullopt;
    }

    ShaderProgram prog{glCreateProgram()};
    glAttachShader(prog.id, vs.handle.id);
    glAttachShader(prog.id, fs.handle.id);
    glLinkProgram(prog.id);

    prog.init_uniform_locations();

    vs.destroy();
    fs.destroy();

    GLint ok{0};
    glGetProgramiv(prog.id, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint log_len = 0;
        glGetProgramiv(prog.id, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<usize>(std::max(1, log_len)), '\0');
        glGetProgramInfoLog(prog.id, log_len, nullptr, log.data());
        std::println(stderr, "Program link failed:\n{}", log);
        glDeleteProgram(prog.id);
        prog.id = 0;
        return std::nullopt;
    }
    return prog;
}

auto read_text_file(const std::string& path) -> std::optional<std::string>
{
    const std::ifstream file(path, std::ios::in);
    if (!file.is_open())
    {
        std::println(stderr, "Shader file not found: '{}'", path);
        return std::nullopt;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    if (file.fail() && !file.eof())
    {
        std::println(stderr, "Shader file IO error while reading: '{}'", path);
        return std::nullopt;
    }

    std::string contents = ss.str();
    if (contents.empty())
    {
        std::println(stderr, "Shader file is empty: '{}'", path);
        return std::nullopt;
    }

    return contents;
}

auto load_shader_sources(const std::string& shader_name) -> std::optional<std::string>
{
    const std::string path{"assets/shaders/" + shader_name};
    return read_text_file(path);
}

auto create_program_from_file(std::string shader_name) -> std::optional<ShaderProgram>
{
    auto frag = load_shader_sources(shader_name + ".frag");
    if (!frag)
    {
        std::println(stderr, "Failed to load fragment shader source for '{}'", shader_name);
        return std::nullopt;
    }

    auto vert = load_shader_sources(shader_name + ".vert");
    if (!vert)
    {
        std::println(stderr, "Failed to load vertex shader source for '{}'", shader_name);
        return std::nullopt;
    }

    auto out = create_program(*vert, *frag);
    if (!out)
    {
        std::println(stderr, "Failed to compile/link shader program '{}'", shader_name);
        return std::nullopt;
    }
    return out;
}

}  // namespace ds_pba
