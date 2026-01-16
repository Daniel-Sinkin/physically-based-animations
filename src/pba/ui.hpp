// pba/ui.hpp
#pragma once

#include "scene_context.hpp"
#include <string_view>

namespace ds_pba {
void apply_blender_style();
void ui_log(std::string_view msg);
void render_imgui_windows(SceneContext& ctx, int frame_counter);
void render_menu_bar(SceneContext& scene_context, SceneHotReloader& scene_reloader);
} // namespace ds_pba