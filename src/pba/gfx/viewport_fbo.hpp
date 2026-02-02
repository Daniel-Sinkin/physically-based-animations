// pba/gfx/viewport_fbo.hpp
#pragma once
#include "pba/core/core_types.hpp"
//
#include <glad/glad.h>
#include <imgui.h>

namespace ds_pba
{

struct ViewportFBO
{
    GLuint fbo{0};
    GLuint color_tex{0};
    GLuint depth_rbo{0};
    int width{0};
    int height{0};

    [[nodiscard]] auto aspect_ratio() const noexcept -> f32;

    auto destroy() noexcept -> void;

    [[nodiscard]] auto ensure_size(int w, int h) noexcept -> bool;
    [[nodiscard]] auto imgui_texture_id() const noexcept -> ImTextureID;
};

}  // namespace ds_pba
