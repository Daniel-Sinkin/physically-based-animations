// pba/ui/ui_theme.hpp
#pragma once

#include "pba/core/core_types.hpp"

#include <filesystem>
#include <imgui.h>
#include <json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace ds_pba
{

enum class ThemeBase
{
    Dark,
    Light,
    Classic,
};

enum class UiThemeError
{
    FileOpenFailed,
    JsonParseError,
    InvalidFormat,
    UnknownColorKey,
    InvalidColorValue,
};

[[nodiscard]] constexpr const char* to_string(UiThemeError e) noexcept
{
    switch (e)
    {
        case UiThemeError::FileOpenFailed:
            return "FileOpenFailed";
        case UiThemeError::JsonParseError:
            return "JsonParseError";
        case UiThemeError::InvalidFormat:
            return "InvalidFormat";
        case UiThemeError::UnknownColorKey:
            return "UnknownColorKey";
        case UiThemeError::InvalidColorValue:
            return "InvalidColorValue";
    }
    return "Unknown";
}

struct ColorAssign
{
    ImGuiCol slot{};
    u32 rgba{};
};

struct UiTheme
{
    std::string name{};
    ThemeBase base{ThemeBase::Dark};

    std::vector<ColorAssign> colors{};
    std::optional<std::string> font_id{};

    bool has_window_rounding{false};
    bool has_child_rounding{false};
    bool has_popup_rounding{false};
    bool has_frame_rounding{false};
    bool has_tab_rounding{false};
    bool has_grab_rounding{false};
    bool has_scrollbar_rounding{false};

    f32 window_rounding{};
    f32 child_rounding{};
    f32 popup_rounding{};
    f32 frame_rounding{};
    f32 tab_rounding{};
    f32 grab_rounding{};
    f32 scrollbar_rounding{};

    bool has_window_border_size{false};
    bool has_child_border_size{false};
    bool has_popup_border_size{false};
    bool has_frame_border_size{false};

    f32 window_border_size{};
    f32 child_border_size{};
    f32 popup_border_size{};
    f32 frame_border_size{};

    bool has_frame_padding{false};
    bool has_item_spacing{false};
    bool has_item_inner_spacing{false};

    ImVec2 frame_padding{};
    ImVec2 item_spacing{};
    ImVec2 item_inner_spacing{};
};

struct UiThemePack
{
    std::vector<UiTheme> themes{};
    std::optional<usize> default_index{};
};

[[nodiscard]] std::optional<UiThemePack> load_theme_pack_json(const std::filesystem::path& path);

void apply_theme(const UiTheme& theme);

void to_json(nlohmann::json& j, const UiTheme& theme);
void from_json(const nlohmann::json& j, UiTheme& theme);

void to_json(nlohmann::json& j, const UiThemePack& pack);
void from_json(const nlohmann::json& j, UiThemePack& pack);

}  // namespace ds_pba
