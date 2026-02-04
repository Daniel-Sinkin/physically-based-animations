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
auto file_exists_regular(const std::filesystem::path& p) noexcept -> bool
{
    namespace fs = std::filesystem;
    std::error_code error_code{};
    if (!fs::exists(p, error_code) || error_code)
    {
        return false;
    }
    if (!fs::is_regular_file(p, error_code) || error_code)
    {
        return false;
    }
    return true;
}
}  // namespace

auto load_image_rgba8(const std::filesystem::path& path, TextureLoadOptions opt)
    -> std::optional<ImageRGBA8>
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

    int w{0}, h{0};
    int comp_in_file{0};

    const auto req_comp = opt.force_rgba ? 4 : 0;
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

    const auto w_u = static_cast<usize>(w);
    const auto h_u = static_cast<usize>(h);
    const auto c_u = static_cast<usize>(out_comp);
    if (w_u > (std::numeric_limits<usize>::max() / h_u))
    {
        stbi_image_free(data);
        std::println(stderr, "Image too large (overflow) for '{}': {}x{}", path.string(), w, h);
        return std::nullopt;
    }
    const auto wh = w_u * h_u;
    if (wh > (std::numeric_limits<usize>::max() / c_u))
    {
        stbi_image_free(data);
        std::println(stderr, "Image too large (overflow) for '{}': {}x{}", path.string(), w, h);
        return std::nullopt;
    }
    const usize nbytes = wh * c_u;

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
