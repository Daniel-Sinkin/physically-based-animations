// pba/core/render_types.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"

namespace ds_pba
{
struct GridSettings
{
    int n_lines_per_side{k_num_lines_per_side};
    f32 spacing{k_spacing};
    f32 fog_start{k_fog_start};
    f32 fog_end{k_fog_end};
    f32 minor_alpha{k_minor_alpha};
    f32 axis_alpha{k_axis_alpha};
};
}  // namespace ds_pba
