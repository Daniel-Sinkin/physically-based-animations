// pba/ui/ui.hpp
#pragma once

#include <string_view>

namespace ds_pba
{
struct GfxContext;
struct PhysicsContext;
struct EngineContext;

void apply_blender_style();
void ui_log(std::string_view msg);
void render_imgui_windows(EngineContext& engine_context);
void render_menu_bar(GfxContext& render_context);
}  // namespace ds_pba
