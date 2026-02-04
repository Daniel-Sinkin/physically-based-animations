// pba/ui/ui_theme.cpp
#include "pba/core/gsl.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/ui/ui_theme.hpp"
//

#include <cctype>
#include <fstream>
#include <json.hpp>

namespace ds_pba
{
namespace
{

constexpr auto is_hex_digit(char c) noexcept -> bool
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

auto parse_rgba_u32_from_string(std::string_view s) noexcept -> std::optional<u32>
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

auto parse_rgba_u32(const nlohmann::json& j) noexcept -> std::optional<u32>
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

auto get_f32(const nlohmann::json& j) noexcept -> std::optional<f32>
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

auto get_vec2(const nlohmann::json& j) noexcept -> std::optional<ImVec2>
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

auto parse_base(std::string_view s) noexcept -> std::optional<ThemeBase>
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

constexpr auto base_to_c_string(ThemeBase b) noexcept -> czstring
{
    switch (b)
    {
        case ThemeBase::Dark:
            return "dark";
        case ThemeBase::Light:
            return "light";
        case ThemeBase::Classic:
            return "classic";
    }
    return "dark";
}

struct ColEntry
{
    std::string_view key;
    ImGuiCol col;
};

static constexpr ColEntry kColMap[] = {
    {"Text", ImGuiCol_Text},
    {"TextDisabled", ImGuiCol_TextDisabled},
    {"WindowBg", ImGuiCol_WindowBg},
    {"ChildBg", ImGuiCol_ChildBg},
    {"PopupBg", ImGuiCol_PopupBg},
    {"Border", ImGuiCol_Border},
    {"BorderShadow", ImGuiCol_BorderShadow},
    {"FrameBg", ImGuiCol_FrameBg},
    {"FrameBgHovered", ImGuiCol_FrameBgHovered},
    {"FrameBgActive", ImGuiCol_FrameBgActive},
    {"TitleBg", ImGuiCol_TitleBg},
    {"TitleBgActive", ImGuiCol_TitleBgActive},
    {"TitleBgCollapsed", ImGuiCol_TitleBgCollapsed},
    {"MenuBarBg", ImGuiCol_MenuBarBg},
    {"Button", ImGuiCol_Button},
    {"ButtonHovered", ImGuiCol_ButtonHovered},
    {"ButtonActive", ImGuiCol_ButtonActive},
    {"Header", ImGuiCol_Header},
    {"HeaderHovered", ImGuiCol_HeaderHovered},
    {"HeaderActive", ImGuiCol_HeaderActive},
    {"Separator", ImGuiCol_Separator},
    {"SeparatorHovered", ImGuiCol_SeparatorHovered},
    {"SeparatorActive", ImGuiCol_SeparatorActive},
    {"Tab", ImGuiCol_Tab},
    {"TabHovered", ImGuiCol_TabHovered},
    {"TabActive", ImGuiCol_TabActive},
    {"TabUnfocused", ImGuiCol_TabUnfocused},
    {"TabUnfocusedActive", ImGuiCol_TabUnfocusedActive},
    {"ScrollbarBg", ImGuiCol_ScrollbarBg},
    {"ScrollbarGrab", ImGuiCol_ScrollbarGrab},
    {"ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
    {"ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
    {"CheckMark", ImGuiCol_CheckMark},
    {"SliderGrab", ImGuiCol_SliderGrab},
    {"SliderGrabActive", ImGuiCol_SliderGrabActive},
    {"TableRowBg", ImGuiCol_TableRowBg},
    {"TableRowBgAlt", ImGuiCol_TableRowBgAlt},
};

std::optional<ImGuiCol> col_from_name(std::string_view name) noexcept
{
    for (const auto& e : kColMap)
    {
        if (e.key == name)
        {
            return e.col;
        }
    }
    return std::nullopt;
}

std::optional<std::string_view> name_from_col(ImGuiCol col) noexcept
{
    for (const auto& e : kColMap)
    {
        if (e.col == col)
        {
            return e.key;
        }
    }
    return std::nullopt;
}

auto rgba_to_hex(u32 rgba) -> std::string
{
    // 0xRRGGBBAA
    return std::format("0x{:08X}", rgba);
}

}  // namespace

auto load_theme_pack_json(const std::filesystem::path& path) -> std::optional<UiThemePack>
{
    std::ifstream f(path);
    if (!f.is_open())
    {
        std::println(stderr, "[UiTheme] Failed to open theme pack: '{}'", path.string());
        return std::nullopt;
    }

    nlohmann::json root = nlohmann::json::parse(f, nullptr, false);
    if (root.is_discarded())
    {
        std::println(stderr, "[UiTheme] JSON parse error in '{}'", path.string());
        return std::nullopt;
    }

    try
    {
        UiThemePack pack = root.get<UiThemePack>();
        return pack;
    }
    catch (const std::exception& e)
    {
        std::println(stderr, "[UiTheme] Invalid theme pack '{}': {}", path.string(), e.what());
        return std::nullopt;
    }
    catch (...)
    {
        std::println(stderr, "[UiTheme] Invalid theme pack '{}': (unknown error)", path.string());
        return std::nullopt;
    }
}

auto apply_theme(const UiTheme& theme) -> void
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
        const u32 r{(a.rgba >> 24u) & 0xFFu};
        const u32 g{(a.rgba >> 16u) & 0xFFu};
        const u32 b{(a.rgba >> 8u) & 0xFFu};
        const u32 al{(a.rgba >> 0u) & 0xFFu};

        const ImU32 imgui_u32 = (al << 24u) | (b << 16u) | (g << 8u) | r;
        c[a.slot] = ImGui::ColorConvertU32ToFloat4(imgui_u32);
    }

    struct ScalarStyleEntry
    {
        bool UiTheme::* has{};
        f32 UiTheme::* src{};
        f32 ImGuiStyle::* dst{};
    };

    struct Vec2StyleEntry
    {
        bool UiTheme::* has{};
        ImVec2 UiTheme::* src{};
        ImVec2 ImGuiStyle::* dst{};
    };

    // clang-format off
    static constexpr ScalarStyleEntry kScalarStyle[] = {
        // rounding
          // Has                              // src                          // dst
        { &UiTheme::has_window_rounding,      &UiTheme::window_rounding,      &ImGuiStyle::WindowRounding      },
        { &UiTheme::has_child_rounding,       &UiTheme::child_rounding,       &ImGuiStyle::ChildRounding       },
        { &UiTheme::has_popup_rounding,       &UiTheme::popup_rounding,       &ImGuiStyle::PopupRounding       },
        { &UiTheme::has_frame_rounding,       &UiTheme::frame_rounding,       &ImGuiStyle::FrameRounding       },
        { &UiTheme::has_tab_rounding,         &UiTheme::tab_rounding,         &ImGuiStyle::TabRounding         },
        { &UiTheme::has_grab_rounding,        &UiTheme::grab_rounding,        &ImGuiStyle::GrabRounding        },
        { &UiTheme::has_scrollbar_rounding,   &UiTheme::scrollbar_rounding,   &ImGuiStyle::ScrollbarRounding   },
        // borders
        { &UiTheme::has_window_border_size,   &UiTheme::window_border_size,   &ImGuiStyle::WindowBorderSize    },
        { &UiTheme::has_child_border_size,    &UiTheme::child_border_size,    &ImGuiStyle::ChildBorderSize     },
        { &UiTheme::has_popup_border_size,    &UiTheme::popup_border_size,    &ImGuiStyle::PopupBorderSize     },
        { &UiTheme::has_frame_border_size,    &UiTheme::frame_border_size,    &ImGuiStyle::FrameBorderSize     },
    };
    static constexpr Vec2StyleEntry kVec2Style[] = {
        { &UiTheme::has_frame_padding,        &UiTheme::frame_padding,        &ImGuiStyle::FramePadding        },
        { &UiTheme::has_item_spacing,         &UiTheme::item_spacing,         &ImGuiStyle::ItemSpacing         },
        { &UiTheme::has_item_inner_spacing,   &UiTheme::item_inner_spacing,   &ImGuiStyle::ItemInnerSpacing    },
    };
    // clang-format on

    for (const auto& e : kScalarStyle)
    {
        if (theme.*(e.has))
        {
            style.*(e.dst) = theme.*(e.src);
        }
    }

    for (const auto& e : kVec2Style)
    {
        if (theme.*(e.has))
        {
            style.*(e.dst) = theme.*(e.src);
        }
    }

    const auto& imgui_io = ImGui::GetIO();
    if (imgui_io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        c[ImGuiCol_WindowBg].w = 1.0f;
    }
}

auto to_json(nlohmann::json& j, const UiTheme& theme) -> void
{
    j = nlohmann::json::object();
    j["name"] = theme.name;
    j["base"] = base_to_c_string(theme.base);

    if (!theme.colors.empty())
    {
        nlohmann::json colors = nlohmann::json::object();
        for (const auto& a : theme.colors)
        {
            const auto name = name_from_col(a.slot);
            if (!name)
            {
                std::println(
                    stderr,
                    "[UiTheme] Serialization: unsupported ImGuiCol slot={}, skipping",
                    static_cast<int>(a.slot)
                );
                continue;
            }
            colors[std::string(*name)] = rgba_to_hex(a.rgba);
        }
        if (!colors.empty())
        {
            j["colors"] = std::move(colors);
        }
    }

    nlohmann::json style = nlohmann::json::object();

    // clang-format off
    if (theme.has_window_rounding)    style["WindowRounding"]    = theme.window_rounding;
    if (theme.has_child_rounding)     style["ChildRounding"]     = theme.child_rounding;
    if (theme.has_popup_rounding)     style["PopupRounding"]     = theme.popup_rounding;
    if (theme.has_frame_rounding)     style["FrameRounding"]     = theme.frame_rounding;
    if (theme.has_tab_rounding)       style["TabRounding"]       = theme.tab_rounding;
    if (theme.has_grab_rounding)      style["GrabRounding"]      = theme.grab_rounding;
    if (theme.has_scrollbar_rounding) style["ScrollbarRounding"] = theme.scrollbar_rounding;

    if (theme.has_window_border_size) style["WindowBorderSize"]  = theme.window_border_size;
    if (theme.has_child_border_size)  style["ChildBorderSize"]   = theme.child_border_size;
    if (theme.has_popup_border_size)  style["PopupBorderSize"]   = theme.popup_border_size;
    if (theme.has_frame_border_size)  style["FrameBorderSize"]   = theme.frame_border_size;
    // clang-format on

    if (theme.has_frame_padding)
    {
        style["FramePadding"] =
            nlohmann::json::array({theme.frame_padding.x, theme.frame_padding.y});
    }

    if (theme.has_item_spacing)
    {
        style["ItemSpacing"] = nlohmann::json::array({theme.item_spacing.x, theme.item_spacing.y});
    }

    if (theme.has_item_inner_spacing)
    {
        style["ItemInnerSpacing"] =
            nlohmann::json::array({theme.item_inner_spacing.x, theme.item_inner_spacing.y});
    }

    if (!style.empty())
    {
        j["style"] = std::move(style);
    }

    if (theme.font_id)
    {
        j["font"] = nlohmann::json::object();
        j["font"]["id"] = *theme.font_id;
    }
}

auto from_json(const nlohmann::json& j, UiTheme& theme) -> void
{
    if (!j.is_object())
    {
        throw nlohmann::json::type_error::create(302, "UiTheme must be object", j);
    }

    UiTheme out{};

    out.name = j.at("name").get<std::string>();

    if (auto it = j.find("base"); it != j.end())
    {
        if (const auto* s = it->get_ptr<const nlohmann::json::string_t*>())
        {
            auto b = parse_base(*s);
            if (!b)
            {
                throw nlohmann::json::type_error::create(302, "UiTheme.base invalid", j);
            }
            out.base = *b;
        }
        else
        {
            throw nlohmann::json::type_error::create(302, "UiTheme.base must be string", j);
        }
    }

    if (auto it = j.find("colors"); it != j.end())
    {
        if (!it->is_object())
        {
            throw nlohmann::json::type_error::create(302, "UiTheme.colors must be object", j);
        }

        for (auto kv = it->begin(); kv != it->end(); ++kv)
        {
            const std::string& key{kv.key()};
            const nlohmann::json& val{kv.value()};

            const auto col = col_from_name(key);
            if (!col)
            {
                throw nlohmann::json::type_error::create(302, "UiTheme.colors has unknown key", j);
            }

            const auto rgba = parse_rgba_u32(val);
            if (!rgba)
            {
                throw nlohmann::json::type_error::create(
                    302, "UiTheme.colors invalid rgba value", j
                );
            }

            out.colors.push_back(ColorAssign{.slot = *col, .rgba = *rgba});
        }
    }

    if (auto it = j.find("style"); it != j.end())
    {
        if (!it->is_object())
        {
            throw nlohmann::json::type_error::create(302, "UiTheme.style must be object", j);
        }
        const auto& s = *it;

        auto read_scalar = [&](const char* key, bool& has, f32& dst) -> void
        {
            if (auto jt = s.find(key); jt != s.end())
            {
                auto v = get_f32(*jt);
                if (!v)
                {
                    throw nlohmann::json::type_error::create(
                        302, "UiTheme.style scalar invalid", j
                    );
                }
                has = true;
                dst = *v;
            }
        };

        auto read_vec2 = [&](const char* key, bool& has, ImVec2& dst) -> void
        {
            if (auto jt = s.find(key); jt != s.end())
            {
                auto v = get_vec2(*jt);
                if (!v)
                {
                    throw nlohmann::json::type_error::create(302, "UiTheme.style vec2 invalid", j);
                }
                has = true;
                dst = *v;
            }
        };

        read_scalar("WindowRounding", out.has_window_rounding, out.window_rounding);
        read_scalar("ChildRounding", out.has_child_rounding, out.child_rounding);
        read_scalar("PopupRounding", out.has_popup_rounding, out.popup_rounding);
        read_scalar("FrameRounding", out.has_frame_rounding, out.frame_rounding);
        read_scalar("TabRounding", out.has_tab_rounding, out.tab_rounding);
        read_scalar("GrabRounding", out.has_grab_rounding, out.grab_rounding);
        read_scalar("ScrollbarRounding", out.has_scrollbar_rounding, out.scrollbar_rounding);

        read_scalar("WindowBorderSize", out.has_window_border_size, out.window_border_size);
        read_scalar("ChildBorderSize", out.has_child_border_size, out.child_border_size);
        read_scalar("PopupBorderSize", out.has_popup_border_size, out.popup_border_size);
        read_scalar("FrameBorderSize", out.has_frame_border_size, out.frame_border_size);

        read_vec2("FramePadding", out.has_frame_padding, out.frame_padding);
        read_vec2("ItemSpacing", out.has_item_spacing, out.item_spacing);
        read_vec2("ItemInnerSpacing", out.has_item_inner_spacing, out.item_inner_spacing);
    }

    if (auto it_font = j.find("font"); it_font != j.end())
    {
        if (!it_font->is_object())
        {
            throw nlohmann::json::type_error::create(302, "UiTheme.font must be object", j);
        }

        if (auto it_id = it_font->find("id"); it_id != it_font->end())
        {
            if (const auto* s = it_id->get_ptr<const nlohmann::json::string_t*>())
            {
                out.font_id = *s;
            }
            else
            {
                throw nlohmann::json::type_error::create(302, "UiTheme.font.id must be string", j);
            }
        }
    }

    theme = std::move(out);
}

auto to_json(nlohmann::json& j, const UiThemePack& pack) -> void
{
    j = nlohmann::json::object();
    j["themes"] = pack.themes;

    if (pack.default_index)
    {
        const usize idx = *pack.default_index;
        if (idx < pack.themes.size())
        {
            j["default"] = pack.themes[idx].name;
        }
        else
        {
            j["default"] = idx;
        }
    }
}

auto from_json(const nlohmann::json& j, UiThemePack& pack) -> void
{
    if (!j.is_object())
    {
        throw nlohmann::json::type_error::create(302, "UiThemePack must be object", j);
    }

    UiThemePack out{};

    const auto it_themes = j.find("themes");
    if (it_themes == j.end() || !it_themes->is_array())
    {
        throw nlohmann::json::type_error::create(302, "UiThemePack.themes must be array", j);
    }

    for (const auto& t : *it_themes)
    {
        out.themes.push_back(t.get<UiTheme>());
    }

    if (out.themes.empty())
    {
        throw nlohmann::json::type_error::create(302, "UiThemePack.themes must be non-empty", j);
    }

    if (auto it_def = j.find("default"); it_def != j.end())
    {
        if (const auto* s = it_def->get_ptr<const nlohmann::json::string_t*>())
        {
            for (usize i{0zu}; i < out.themes.size(); ++i)
            {
                if (out.themes[i].name == *s)
                {
                    out.default_index = i;
                    break;
                }
            }
        }
        else if (const auto* u = it_def->get_ptr<const nlohmann::json::number_unsigned_t*>())
        {
            const usize idx = static_cast<usize>(*u);
            if (idx < out.themes.size())
            {
                out.default_index = idx;
            }
        }
        else if (const auto* i = it_def->get_ptr<const nlohmann::json::number_integer_t*>())
        {
            if (*i >= 0)
            {
                const usize idx = static_cast<usize>(*i);
                if (idx < out.themes.size())
                {
                    out.default_index = idx;
                }
            }
        }
    }

    pack = std::move(out);
}

}  // namespace ds_pba
