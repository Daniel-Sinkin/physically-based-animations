// pba/assets/texture.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/assets/texture.hpp"
//
#include <print>
#include <stb_image.h>

namespace ds_pba
{
namespace
{
[[nodiscard]] bool file_exists_regular(const std::filesystem::path& p) noexcept
{
    namespace fs = std::filesystem;
    std::error_code ec{};
    if (!fs::exists(p, ec) || ec)
    {
        return false;
    }
    if (!fs::is_regular_file(p, ec) || ec)
    {
        return false;
    }
    return true;
}
}  // namespace

std::optional<ImageRGBA8>
load_image_rgba8(const std::filesystem::path& path, TextureLoadOptions opt)
{
    namespace fs = std::filesystem;

    if (!file_exists_regular(path))
    {
        std::error_code ec{};
        if (!fs::exists(path, ec) || ec)
        {
            std::println(stderr, "Texture file not found: '{}'", path.string());
        }
        else
        {
            std::println(stderr, "Texture path is not a regular file: '{}'", path.string());
        }
        return std::nullopt;
    }

    stbi_set_flip_vertically_on_load(opt.flip_y ? 1 : 0);

    int w{0};
    int h{0};
    int comp_in_file{0};

    const int req_comp = opt.force_rgba ? 4 : 0;

    stbi_uc* data = stbi_load(path.string().c_str(), &w, &h, &comp_in_file, req_comp);
    if (!data)
    {
        std::println(stderr, "stbi_load failed for '{}': {}", path.string(), stbi_failure_reason());
        return std::nullopt;
    }

    if (w <= 0 || h <= 0)
    {
        stbi_image_free(data);
        std::println(stderr, "Invalid image dimensions for '{}': {}x{}", path.string(), w, h);
        return std::nullopt;
    }

    const int out_comp = opt.force_rgba ? 4 : comp_in_file;
    if (out_comp != 4)
    {
        stbi_image_free(data);
        std::println(
            stderr, "Only support RGBA8 but got {} channels for '{}'", out_comp, path.string()
        );
        return std::nullopt;
    }

    const usize nbytes =
        static_cast<usize>(w) * static_cast<usize>(h) * static_cast<usize>(out_comp);

    ImageRGBA8 out{};
    out.width = w;
    out.height = h;
    out.channels = 4;
    out.pixels.resize(nbytes);
    std::memcpy(out.pixels.data(), data, nbytes);

    stbi_image_free(data);

    if (!out.valid())
    {
        std::println(stderr, "Decoded image invalid for '{}'", path.string());
        return std::nullopt;
    }

    return out;
}

}  // namespace ds_pba
