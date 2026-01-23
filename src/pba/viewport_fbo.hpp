// pba/viewport_fbo.hpp
#pragma once

#include "pba/core_types.hpp"

#include <algorithm>
#include <cstdint>
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

    [[nodiscard]] f32 aspect_ratio() const noexcept
    {
        if (height <= 0)
        {
            return 1.0f;
        }
        return static_cast<f32>(width) / static_cast<f32>(height);
    }

    void destroy() noexcept
    {
        if (depth_rbo != 0)
        {
            glDeleteRenderbuffers(1, &depth_rbo);
            depth_rbo = 0;
        }
        if (color_tex != 0)
        {
            glDeleteTextures(1, &color_tex);
            color_tex = 0;
        }
        if (fbo != 0)
        {
            glDeleteFramebuffers(1, &fbo);
            fbo = 0;
        }
        width = 0;
        height = 0;
    }

    [[nodiscard]] bool ensure_size(int w, int h) noexcept
    {
        w = std::max(1, w);
        h = std::max(1, h);

        if (w == width && h == height && fbo != 0 && color_tex != 0 && depth_rbo != 0)
        {
            return true;
        }

        destroy();

        width = w;
        height = h;

        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenTextures(1, &color_tex);
        glBindTexture(GL_TEXTURE_2D, color_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
        );
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);

        glGenRenderbuffers(1, &depth_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_rbo
        );

        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return status == GL_FRAMEBUFFER_COMPLETE;
    }

    [[nodiscard]] ImTextureID imgui_texture_id() const noexcept
    {
        return static_cast<ImTextureID>(static_cast<std::uintptr_t>(color_tex));
    }
};

}  // namespace ds_pba
