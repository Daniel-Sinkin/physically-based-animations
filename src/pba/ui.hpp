// pba/ui.hpp
#pragma once

#include "pba/render_context.hpp"

#include <string_view>

namespace ds_pba
{
struct GridSettings;

void apply_blender_style();
void ui_log(std::string_view msg);
void render_imgui_windows(RenderContext& render_context);
void render_menu_bar(RenderContext& render_context);
}  // namespace ds_pba