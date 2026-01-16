// pba/mesh.hpp
#pragma once

#include "types.hpp"

namespace ds_pba {

GLMesh create_cube_mesh();
GLMesh create_grid_mesh(int n_lines_per_side, f32 spacing, f32 axis_alpha, f32 minor_alpha);

} // namespace ds_pba