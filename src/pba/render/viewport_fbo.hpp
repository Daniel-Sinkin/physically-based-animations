// pba/render/viewport_fbo.hpp
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

    [[nodiscard]] f32 aspect_ratio() const noexcept;

    void destroy() noexcept;

    [[nodiscard]] bool ensure_size(int w, int h) noexcept;

    [[nodiscard]] ImTextureID imgui_texture_id() const noexcept;
};

}  // namespace ds_pba
