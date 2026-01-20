// pba/ui.hpp
#pragma once

#include <string_view>

namespace ds_pba
{
struct RenderContext;
struct PhysicsContext;

void apply_blender_style();
void ui_log(std::string_view msg);
void render_imgui_windows(RenderContext& render_context, PhysicsContext& physics_context);
void render_menu_bar(RenderContext& render_context);
}  // namespace ds_pba
