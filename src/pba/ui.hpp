// pba/ui.hpp
#pragma once

#include "pba/scene_context.hpp"

#include <string_view>

namespace ds_pba {
struct GridSettings;

void apply_blender_style();
void ui_log(std::string_view msg);
void render_imgui_windows(SceneContext &scene_context, ColorRGBAf &background_color, GridSettings &grid, int frame_counter);
void render_menu_bar(SceneContext &scene_context);
} // namespace ds_pba