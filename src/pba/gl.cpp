// pba/gl.cpp
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gl.hpp"
//
#include "pba/gl_shader.hpp"

namespace ds_pba
{
std::optional<ShaderProgram>
create_program(const std::string& vert_src, const std::string& frag_src)
{
    const Shader vs = Shader::create_and_compile(ShaderType::Vertex, vert_src);
    if (!vs.valid() || !vs.compiled_ok())
    {
        std::println(stderr, "Failed Vertex Shader Creation");
        return std::nullopt;
    }
    const Shader fs = Shader::create_and_compile(ShaderType::Fragment, frag_src);
    if (!fs.valid() || !fs.compiled_ok())
    {
        std::println(stderr, "Failed Fragment Shader Creation");
        return std::nullopt;
    }

    ShaderProgram prog{glCreateProgram()};
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok{0};
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint log_len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<usize>(std::max(1, log_len)), '\0');
        glGetProgramInfoLog(prog, log_len, nullptr, log.data());
        std::println(stderr, "Program link failed:\n{}", log);
        glDeleteProgram(prog.id);
        prog.id = 0;
        return std::nullopt;
    }
    return prog;
}

std::expected<std::string, ShaderCreateError> read_text_file(const std::string& path)
{
    const std::ifstream file(path, std::ios::in);
    if (!file.is_open())
    {
        return std::unexpected(ShaderCreateError::FileNotFound);
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    if (file.fail() && !file.eof())
    {
        return std::unexpected(ShaderCreateError::IOError);
    }

    std::string contents = ss.str();
    if (contents.empty())
    {
        return std::unexpected(ShaderCreateError::EmptyFile);
    }

    return contents;
}

std::expected<std::string, ShaderCreateError> load_shader_sources(const std::string& shader_name)
{
    const std::string path = "assets/shaders/" + shader_name;
    auto shader = read_text_file(path);
    if (!shader)
    {
        return std::unexpected(shader.error());
    }
    return std::move(*shader);
}

std::expected<ShaderProgram, ShaderCreateError> create_program_from_file(std::string shader_name)
{
    auto frag = load_shader_sources(shader_name + ".frag");
    if (!frag)
    {
        return std::unexpected(frag.error());
    }

    auto vert = load_shader_sources(shader_name + ".vert");
    if (!vert)
    {
        return std::unexpected(vert.error());
    }

    auto out = create_program(*vert, *frag);
    if (!out.has_value())
    {
        return std::unexpected(ShaderCreateError::CompilationError);
    }
    return *out;
}

}  // namespace ds_pba
