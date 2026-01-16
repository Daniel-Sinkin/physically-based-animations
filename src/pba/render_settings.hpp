// pba/render_settings.hpp
#pragma once

#include "types.hpp"

namespace ds_pba {

struct RenderSettings {
    ColorRGBAf background_color{0.255f, 0.255f, 0.255f, 1.0f};

    struct GridSettings {
        int n_lines_per_side = 30;
        f32 spacing = 1.0f;
        f32 fog_start = 12.0f;
        f32 fog_end = 30.0f;
        f32 minor_alpha = 0.35f;
        f32 axis_alpha = 0.95f;
    } grid{};
};

extern RenderSettings g_render_settings;

} // namespace ds_pba