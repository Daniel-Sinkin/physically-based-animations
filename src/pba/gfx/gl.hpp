// pba/gfx/gl.hpp
#pragma once

#include "pba/gfx/shader_program.hpp"
//
#include <cassert>
#include <string>
//
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace ds_pba
{

[[nodiscard]] auto create_program(const std::string& vert_src, const std::string& frag_src)
    -> std::optional<ShaderProgram>;

[[nodiscard]]
auto create_program_from_file(std::string shader_name) -> std::optional<ShaderProgram>;
[[nodiscard]]
auto read_text_file(const std::string& path) -> std::optional<std::string>;
[[nodiscard]]
auto load_shader_sources(const std::string& shader_name) -> std::optional<std::string>;
}  // namespace ds_pba
