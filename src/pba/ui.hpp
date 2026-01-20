// pba/ui.hpp
#pragma once

#include <string_view>

namespace ds_pba
{
struct RenderContext;

void apply_blender_style();
void ui_log(std::string_view msg);
void render_imgui_windows(RenderContext& render_context);
void render_menu_bar(RenderContext& render_context);
}  // namespace ds_pba
