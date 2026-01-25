// pba/render/gl.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/render/gl_types.hpp"
//
#include <cassert>
#include <expected>
#include <print>
#include <string>
#include <unordered_map>
//
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

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

namespace detail
{

struct UniformLocationCache
{
    // program -> (name -> loc)
    // Locations are uniquely defined by their program and (uniform) name
    // it's simpler to have nested hashmaps than to (for example) hash a
    // std::pair of them, esp. given that we have an explicit hierarchy anyway
    std::unordered_map<ProgramHandle, std::unordered_map<std::string, UniformLocation>> locs{};
#ifndef NDEBUG
    // warn-once set
    std::unordered_map<ProgramHandle, std::unordered_map<std::string, bool>> warned{};
#endif

    [[nodiscard]] UniformLocation get(ProgramHandle program, const char* name)
    {
        auto& per_prog{locs[program]};
        if (auto it = per_prog.find(name); it != per_prog.end())
        {
            return it->second;
        }

        const UniformLocation loc{glGetUniformLocation(program.id, name)};
        per_prog.emplace(name, loc);
        return loc;
    }

    void invalidate_program(ProgramHandle program) noexcept
    {
        locs.erase(program);
#ifndef NDEBUG
        warned.erase(program);
#endif
    }
};

inline UniformLocationCache& uniform_cache()
{
    static UniformLocationCache c{};
    return c;
}

inline void warn_uniform_missing_once(ProgramHandle program, const char* name) noexcept
{
#ifndef NDEBUG
    auto& warned_prog = uniform_cache().warned[program];
    if (auto it = warned_prog.find(name); it != warned_prog.end() && it->second)
    {
        return;
    }
    warned_prog[std::string{name}] = true;

    std::println(
        stderr,
        "[Warning] Uniform '{}' not found in program {}. "
        "Likely optimized out or name mismatch. Skipping update.",
        name,
        static_cast<u32>(program.id)
    );
#else
    (void) program;
    (void) name;
#endif
}

[[nodiscard]] inline UniformLocation
uniform_loc_or_warn(ProgramHandle program, const char* name) noexcept
{
    const UniformLocation loc{uniform_cache().get(program, name)};
    if (loc < 0)
    {
        warn_uniform_missing_once(program, name);
    }
    return loc;
}

}  // namespace detail

inline void invalidate_uniform_cache_for_program(ProgramHandle program) noexcept
{
    // THe uniforms aren't stable after relinking so caches need to be invalidated on shutdown
    detail::uniform_cache().invalidate_program(program);
}

inline void set_uniform_mat4(ProgramHandle program, const char* name, const glm::mat4& m) noexcept
{
    const UniformLocation loc{detail::uniform_loc_or_warn(program, name)};
    if (loc < 0)
    {
        return;
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
}

inline void set_uniform_vec3(ProgramHandle program, const char* name, const glm::vec3& v) noexcept
{
    const UniformLocation loc{detail::uniform_loc_or_warn(program, name)};
    if (loc < 0)
    {
        return;
    }
    glUniform3f(loc, v.x, v.y, v.z);
}

inline void set_uniform_float(ProgramHandle program, const char* name, f32 v) noexcept
{
    const UniformLocation loc{detail::uniform_loc_or_warn(program, name)};
    if (loc < 0)
    {
        return;
    }
    glUniform1f(loc, v);
}

inline void set_uniform_bool(ProgramHandle program, const char* name, bool v) noexcept
{
    const UniformLocation loc{detail::uniform_loc_or_warn(program, name)};
    if (loc < 0)
    {
        return;
    }
    glUniform1i(loc, v ? 1 : 0);
}

}  // namespace ds_pba
