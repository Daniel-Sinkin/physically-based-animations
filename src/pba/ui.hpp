// pba/ui.hpp
#pragma once

#include <optional>

#include "types.hpp"
#include "scene_context.hpp"

namespace ds_pba {

void render_imgui_windows(SceneContext& ctx, std::optional<usize> &selected_index, int frame_counter);

} // namespace ds_pba