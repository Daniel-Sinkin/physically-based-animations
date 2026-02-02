// pba/ui/ui.hpp
#pragma once

#include <string_view>

namespace ds_pba
{
struct GfxContext;
struct PhysicsContext;
struct EngineContext;

auto apply_blender_style() -> void;
auto ui_log(std::string_view msg) -> void;
auto render_imgui_windows(EngineContext& engine_context) -> void;
auto render_menu_bar(GfxContext& render_context) -> void;
}  // namespace ds_pba
