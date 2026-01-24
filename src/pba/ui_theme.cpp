// pba/ui_theme.cpp
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/ui_theme.hpp"
//

#include <cctype>
#include <fstream>
#include <json.hpp>

namespace ds_pba::ui_theme
{
namespace
{

constexpr bool is_hex_digit(char c) noexcept
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

std::optional<u32> parse_rgba_u32_from_string(std::string_view s) noexcept
{
    // 0xRRGGBBAA or RRGGBBAA
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
    {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
    {
        s.remove_suffix(1);
    }

    if (s.starts_with("0x") || s.starts_with("0X"))
    {
        s.remove_prefix(2);
    }

    if (s.size() != 8)
    {
        return std::nullopt;
    }

    for (const char c : s)
    {
        if (!is_hex_digit(c))
        {
            return std::nullopt;
        }
    }

    auto hex = [](char c) -> u32
    {
        if (c >= '0' && c <= '9')
            return static_cast<u32>(c - '0');
        if (c >= 'a' && c <= 'f')
            return static_cast<u32>(10 + (c - 'a'));
        return static_cast<u32>(10 + (c - 'A'));
    };

    u32 v{0u};
    for (const char c : s)
    {
        v = (v << 4u) | hex(c);
    }
    return v;
}

std::optional<u32> parse_rgba_u32(const nlohmann::json& j) noexcept
{
    if (const auto* u = j.get_ptr<const nlohmann::json::number_unsigned_t*>())
    {
        return static_cast<u32>(*u);
    }
    if (const auto* i = j.get_ptr<const nlohmann::json::number_integer_t*>())
    {
        if (*i < 0)
        {
            return std::nullopt;
        }
        return static_cast<u32>(*i);
    }
    if (const auto* s = j.get_ptr<const nlohmann::json::string_t*>())
    {
        return parse_rgba_u32_from_string(*s);
    }
    return std::nullopt;
}

std::optional<f32> get_f32(const nlohmann::json& j) noexcept
{
    if (const auto* f = j.get_ptr<const nlohmann::json::number_float_t*>())
    {
        return static_cast<f32>(*f);
    }
    if (const auto* i = j.get_ptr<const nlohmann::json::number_integer_t*>())
    {
        return static_cast<f32>(*i);
    }
    if (const auto* u = j.get_ptr<const nlohmann::json::number_unsigned_t*>())
    {
        return static_cast<f32>(*u);
    }
    return std::nullopt;
}

std::optional<ImVec2> get_vec2(const nlohmann::json& j) noexcept
{
    if (!j.is_array() || j.size() != 2)
    {
        return std::nullopt;
    }
    const auto fx = get_f32(j[0]);
    const auto fy = get_f32(j[1]);
    if (!fx || !fy)
    {
        return std::nullopt;
    }
    return ImVec2{*fx, *fy};
}

std::optional<ThemeBase> parse_base(std::string_view s) noexcept
{
    auto lower = [](std::string_view in) -> std::string
    {
        std::string out;
        out.reserve(in.size());
        for (const char c : in)
        {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        return out;
    };

    const std::string v{lower(s)};
    if (v == "dark")
    {
        return ThemeBase::Dark;
    }
    if (v == "light")
    {
        return ThemeBase::Light;
    }
    if (v == "classic")
    {
        return ThemeBase::Classic;
    }
    return std::nullopt;
}

std::optional<ImGuiCol> col_from_name(std::string_view name) noexcept
{
    // Keep this list explicit and extend as needed.
    // Names match ImGuiCol enum tokens without the "ImGuiCol_" prefix.
    struct Entry
    {
        std::string_view key;
        ImGuiCol col;
    };

    // clang-format off
    static constexpr Entry kMap[] = {
        {"Text",                  ImGuiCol_Text},
        {"TextDisabled",          ImGuiCol_TextDisabled},
        {"WindowBg",              ImGuiCol_WindowBg},
        {"ChildBg",               ImGuiCol_ChildBg},
        {"PopupBg",               ImGuiCol_PopupBg},
        {"Border",                ImGuiCol_Border},
        {"BorderShadow",          ImGuiCol_BorderShadow},
        {"FrameBg",               ImGuiCol_FrameBg},
        {"FrameBgHovered",        ImGuiCol_FrameBgHovered},
        {"FrameBgActive",         ImGuiCol_FrameBgActive},
        {"TitleBg",               ImGuiCol_TitleBg},
        {"TitleBgActive",         ImGuiCol_TitleBgActive},
        {"TitleBgCollapsed",      ImGuiCol_TitleBgCollapsed},
        {"MenuBarBg",             ImGuiCol_MenuBarBg},
        {"Button",                ImGuiCol_Button},
        {"ButtonHovered",         ImGuiCol_ButtonHovered},
        {"ButtonActive",          ImGuiCol_ButtonActive},
        {"Header",                ImGuiCol_Header},
        {"HeaderHovered",         ImGuiCol_HeaderHovered},
        {"HeaderActive",          ImGuiCol_HeaderActive},
        {"Separator",             ImGuiCol_Separator},
        {"SeparatorHovered",      ImGuiCol_SeparatorHovered},
        {"SeparatorActive",       ImGuiCol_SeparatorActive},
        {"Tab",                   ImGuiCol_Tab},
        {"TabHovered",            ImGuiCol_TabHovered},
        {"TabActive",             ImGuiCol_TabActive},
        {"TabUnfocused",          ImGuiCol_TabUnfocused},
        {"TabUnfocusedActive",    ImGuiCol_TabUnfocusedActive},
        {"ScrollbarBg",           ImGuiCol_ScrollbarBg},
        {"ScrollbarGrab",         ImGuiCol_ScrollbarGrab},
        {"ScrollbarGrabHovered",  ImGuiCol_ScrollbarGrabHovered},
        {"ScrollbarGrabActive",   ImGuiCol_ScrollbarGrabActive},
        {"CheckMark",             ImGuiCol_CheckMark},
        {"SliderGrab",            ImGuiCol_SliderGrab},
        {"SliderGrabActive",      ImGuiCol_SliderGrabActive},
        {"TableRowBg",            ImGuiCol_TableRowBg},
        {"TableRowBgAlt",         ImGuiCol_TableRowBgAlt},
    };
    // clang-format on

    for (const auto& e : kMap)
    {
        if (e.key == name)
        {
            return e.col;
        }
    }
    return std::nullopt;
}

std::expected<UiTheme, UiThemeError> parse_theme(const nlohmann::json& j)
{
    if (!j.is_object())
    {
        return std::unexpected(UiThemeError::InvalidFormat);
    }

    const auto* name = j.find("name") != j.end()
                           ? j.at("name").get_ptr<const nlohmann::json::string_t*>()
                           : nullptr;
    if (!name)
    {
        return std::unexpected(UiThemeError::InvalidFormat);
    }

    UiTheme out{};
    out.name = *name;

    if (auto it = j.find("base"); it != j.end())
    {
        if (const auto* s = it->get_ptr<const nlohmann::json::string_t*>())
        {
            if (auto b = parse_base(*s))
            {
                out.base = *b;
            }
            else
            {
                return std::unexpected(UiThemeError::InvalidFormat);
            }
        }
        else
        {
            return std::unexpected(UiThemeError::InvalidFormat);
        }
    }

    // colors: object { "Text": "0xE6E6E6FF", ... }
    if (auto it = j.find("colors"); it != j.end())
    {
        if (!it->is_object())
        {
            return std::unexpected(UiThemeError::InvalidFormat);
        }

        for (auto kv = it->begin(); kv != it->end(); ++kv)
        {
            const std::string& key{kv.key()};
            const nlohmann::json& val{kv.value()};

            const auto col = col_from_name(key);
            if (!col)
            {
                return std::unexpected(UiThemeError::UnknownColorKey);
            }

            const auto rgba = parse_rgba_u32(val);
            if (!rgba)
            {
                return std::unexpected(UiThemeError::InvalidColorValue);
            }

            out.colors.push_back(ColorAssign{.slot = *col, .rgba = *rgba});
        }
    }

    if (auto it = j.find("style"); it != j.end())
    {
        if (!it->is_object())
        {
            return std::unexpected(UiThemeError::InvalidFormat);
        }
        const auto& s = *it;

        auto set_scalar =
            [&](const char* key, bool& has, f32& dst) -> std::expected<void, UiThemeError>
        {
            if (auto jt = s.find(key); jt != s.end())
            {
                auto v = get_f32(*jt);
                if (!v)
                {
                    return std::unexpected(UiThemeError::InvalidFormat);
                }
                has = true;
                dst = *v;
            }
            return {};
        };

        auto set_vec2 =
            [&](const char* key, bool& has, ImVec2& dst) -> std::expected<void, UiThemeError>
        {
            if (auto jt = s.find(key); jt != s.end())
            {
                auto v = get_vec2(*jt);
                if (!v)
                {
                    return std::unexpected(UiThemeError::InvalidFormat);
                }
                has = true;
                dst = *v;
            }
            return {};
        };

        if (!set_scalar("WindowRounding", out.has_window_rounding, out.window_rounding))
            return std::unexpected(UiThemeError::InvalidFormat);
        if (!set_scalar("ChildRounding", out.has_child_rounding, out.child_rounding))
            return std::unexpected(UiThemeError::InvalidFormat);
        if (!set_scalar("PopupRounding", out.has_popup_rounding, out.popup_rounding))
            return std::unexpected(UiThemeError::InvalidFormat);
        if (!set_scalar("FrameRounding", out.has_frame_rounding, out.frame_rounding))
            return std::unexpected(UiThemeError::InvalidFormat);
        if (!set_scalar("TabRounding", out.has_tab_rounding, out.tab_rounding))
            return std::unexpected(UiThemeError::InvalidFormat);
        if (!set_scalar("GrabRounding", out.has_grab_rounding, out.grab_rounding))
            return std::unexpected(UiThemeError::InvalidFormat);
        if (!set_scalar("ScrollbarRounding", out.has_scrollbar_rounding, out.scrollbar_rounding))
            return std::unexpected(UiThemeError::InvalidFormat);

        if (!set_scalar("WindowBorderSize", out.has_window_border_size, out.window_border_size))
            return std::unexpected(UiThemeError::InvalidFormat);
        if (!set_scalar("ChildBorderSize", out.has_child_border_size, out.child_border_size))
            return std::unexpected(UiThemeError::InvalidFormat);
        if (!set_scalar("PopupBorderSize", out.has_popup_border_size, out.popup_border_size))
            return std::unexpected(UiThemeError::InvalidFormat);
        if (!set_scalar("FrameBorderSize", out.has_frame_border_size, out.frame_border_size))
            return std::unexpected(UiThemeError::InvalidFormat);

        if (!set_vec2("FramePadding", out.has_frame_padding, out.frame_padding))
            return std::unexpected(UiThemeError::InvalidFormat);
        if (!set_vec2("ItemSpacing", out.has_item_spacing, out.item_spacing))
            return std::unexpected(UiThemeError::InvalidFormat);
        if (!set_vec2("ItemInnerSpacing", out.has_item_inner_spacing, out.item_inner_spacing))
            return std::unexpected(UiThemeError::InvalidFormat);
    }
    if (auto it_font = j.find("font"); it_font != j.end())
    {
        if (!it_font->is_object())
        {
            return std::unexpected(UiThemeError::InvalidFormat);
        }

        if (auto it_id = it_font->find("id"); it_id != it_font->end())
        {
            if (const auto* s = it_id->get_ptr<const nlohmann::json::string_t*>())
            {
                out.font_id = *s;
            }
            else
            {
                return std::unexpected(UiThemeError::InvalidFormat);
            }
        }
    }

    return out;
}

}  // namespace

std::expected<UiThemePack, UiThemeError> load_theme_pack_json(const std::filesystem::path& path)
{
    std::ifstream f(path);
    if (!f.is_open())
    {
        return std::unexpected(UiThemeError::FileOpenFailed);
    }

    nlohmann::json root = nlohmann::json::parse(f, nullptr, false);
    if (root.is_discarded())
    {
        return std::unexpected(UiThemeError::JsonParseError);
    }
    if (!root.is_object())
    {
        return std::unexpected(UiThemeError::InvalidFormat);
    }

    UiThemePack pack{};

    const auto it_themes = root.find("themes");
    if (it_themes == root.end() || !it_themes->is_array())
    {
        return std::unexpected(UiThemeError::InvalidFormat);
    }

    for (const auto& t : *it_themes)
    {
        auto parsed = parse_theme(t);
        if (!parsed)
        {
            return std::unexpected(parsed.error());
        }
        pack.themes.push_back(std::move(*parsed));
    }

    if (pack.themes.empty())
    {
        return std::unexpected(UiThemeError::InvalidFormat);
    }

    if (auto it_def = root.find("default"); it_def != root.end())
    {
        if (const auto* s = it_def->get_ptr<const nlohmann::json::string_t*>())
        {
            for (usize i{0}; i < pack.themes.size(); ++i)
            {
                if (pack.themes[i].name == *s)
                {
                    pack.default_index = i;
                    break;
                }
            }
        }
        else if (const auto* u = it_def->get_ptr<const nlohmann::json::number_unsigned_t*>())
        {
            const usize idx = static_cast<usize>(*u);
            if (idx < pack.themes.size())
            {
                pack.default_index = idx;
            }
        }
    }

    return pack;
}

void apply_theme(const UiTheme& theme)
{
    switch (theme.base)
    {
        case ThemeBase::Dark:
            ImGui::StyleColorsDark();
            break;
        case ThemeBase::Light:
            ImGui::StyleColorsLight();
            break;
        case ThemeBase::Classic:
            ImGui::StyleColorsClassic();
            break;
    }

    ImGuiStyle& style{ImGui::GetStyle()};
    ImVec4* c{style.Colors};

    for (const auto& a : theme.colors)
    {
        const u32 r = (a.rgba >> 24u) & 0xFFu;
        const u32 g = (a.rgba >> 16u) & 0xFFu;
        const u32 b = (a.rgba >> 8u) & 0xFFu;
        const u32 al = (a.rgba >> 0u) & 0xFFu;

        const ImU32 imgui_u32 = (al << 24u) | (b << 16u) | (g << 8u) | r;
        c[a.slot] = ImGui::ColorConvertU32ToFloat4(imgui_u32);
    }

    if (theme.has_window_rounding)
        style.WindowRounding = theme.window_rounding;
    if (theme.has_child_rounding)
        style.ChildRounding = theme.child_rounding;
    if (theme.has_popup_rounding)
        style.PopupRounding = theme.popup_rounding;
    if (theme.has_frame_rounding)
        style.FrameRounding = theme.frame_rounding;
    if (theme.has_tab_rounding)
        style.TabRounding = theme.tab_rounding;
    if (theme.has_grab_rounding)
        style.GrabRounding = theme.grab_rounding;
    if (theme.has_scrollbar_rounding)
        style.ScrollbarRounding = theme.scrollbar_rounding;

    if (theme.has_window_border_size)
        style.WindowBorderSize = theme.window_border_size;
    if (theme.has_child_border_size)
        style.ChildBorderSize = theme.child_border_size;
    if (theme.has_popup_border_size)
        style.PopupBorderSize = theme.popup_border_size;
    if (theme.has_frame_border_size)
        style.FrameBorderSize = theme.frame_border_size;

    if (theme.has_frame_padding)
        style.FramePadding = theme.frame_padding;
    if (theme.has_item_spacing)
        style.ItemSpacing = theme.item_spacing;
    if (theme.has_item_inner_spacing)
        style.ItemInnerSpacing = theme.item_inner_spacing;

    const ImGuiIO& io{ImGui::GetIO()};
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        c[ImGuiCol_WindowBg].w = 1.0f;
    }
}
}  // namespace ds_pba::ui_theme
