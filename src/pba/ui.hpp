// pba/ui.hpp
#pragma once

#include <optional>
#include <vector>

#include "camera.hpp"
#include "types.hpp"

namespace ds_pba {

void render_imgui_windows(Camera &cam, std::vector<Object> &objects, std::optional<usize> &selected_index);

} // namespace ds_pba