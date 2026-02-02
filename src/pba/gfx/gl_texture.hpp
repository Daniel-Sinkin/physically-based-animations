// pba/gfx/texture_gl.hpp
#pragma once

#include "pba/assets/texture.hpp"
#include "pba/core/core_types.hpp"

#include <expected>

namespace ds_pba
{

enum class GLTextureError : u8
{
    InvalidImage,
    OpenGLError,
};

struct GLTexture2D
{
    GLuint id{0};
    int width{0};
    int height{0};

    [[nodiscard]] auto valid() const noexcept -> bool
    {
        return id != 0 && width > 0 && height > 0;
    }

    auto destroy() noexcept -> void
    {
        if (id != 0)
        {
            glDeleteTextures(1, &id);
            id = 0;
            width = 0;
            height = 0;
        }
    }
};

struct GLTextureUploadOptions
{
    bool generate_mips{true};
    bool srgb{false};
};

[[nodiscard]] auto upload_texture_2d_rgba8(const ImageRGBA8& img, GLTextureUploadOptions opt = {})
    -> std::expected<GLTexture2D, GLTextureError>;

}  // namespace ds_pba
