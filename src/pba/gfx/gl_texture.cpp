// pba/gfx/texture_gl.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gl_texture.hpp"
//

namespace ds_pba
{

std::expected<GLTexture2D, GLTextureError>
upload_texture_2d_rgba8(const ImageRGBA8& img, GLTextureUploadOptions opt)
{
    if (!img.valid())
    {
        return std::unexpected(GLTextureError::InvalidImage);
    }

    GLuint tex{0};
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        opt.generate_mips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLint prev_unpack{0};
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_unpack);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    const GLint internal_fmt = opt.srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internal_fmt,
        img.width,
        img.height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        img.pixels.data()
    );

    if (opt.generate_mips)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, prev_unpack);
    glBindTexture(GL_TEXTURE_2D, 0);

    const GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        glDeleteTextures(1, &tex);
        return std::unexpected(GLTextureError::OpenGLError);
    }

    return GLTexture2D{.id = tex, .width = img.width, .height = img.height};
}

void destroy_texture(GLTexture2D& tex) noexcept
{
    if (tex.id != 0)
    {
        glDeleteTextures(1, &tex.id);
        tex.id = 0;
        tex.width = 0;
        tex.height = 0;
    }
}

}  // namespace ds_pba
