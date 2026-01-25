// pba/ui/ui.hpp
#pragma once

#include <string_view>

namespace ds_pba
{
struct Renderer;
struct PhysicsContext;
struct EngineContext;

void apply_blender_style();
void ui_log(std::string_view msg);
void render_imgui_windows(EngineContext& engine_context);
void render_menu_bar(Renderer& render_context);
}  // namespace ds_pba
