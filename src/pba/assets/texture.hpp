// pba/assets/texture.hpp
#pragma once

#include "pba/core/core_types.hpp"

#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace ds_pba
{

struct ImageRGBA8
{
    int width{0};
    int height{0};
    int channels{4};
    std::vector<u8> pixels{};

    [[nodiscard]] bool valid() const noexcept
    {
        if (width <= 0 || height <= 0 || channels != 4)
        {
            return false;
        }
        const usize expected = static_cast<usize>(width) * static_cast<usize>(height) * 4zu;
        return pixels.size() == expected;
    }

    [[nodiscard]] std::span<const u8> bytes() const noexcept
    {
        return std::span<const u8>{pixels.data(), pixels.size()};
    }
};

struct TextureLoadOptions
{
    // Not threadsafe
    bool flip_y{true};
    bool force_rgba{true};
};

[[nodiscard]] auto load_image_rgba8(const std::filesystem::path& path, TextureLoadOptions opt = {})
    -> std::optional<ImageRGBA8>;

}  // namespace ds_pba
