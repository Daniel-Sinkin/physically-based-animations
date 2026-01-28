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

[[nodiscard]] std::optional<ShaderProgram>
create_program(const std::string& vert_src, const std::string& frag_src);

[[nodiscard]] std::optional<ShaderProgram> create_program_from_file(std::string shader_name);
[[nodiscard]] std::optional<std::string> read_text_file(const std::string& path);
[[nodiscard]] std::optional<std::string> load_shader_sources(const std::string& shader_name);
}  // namespace ds_pba
