// pba/mesh.hpp
#pragma once

#include "pba/core_types.hpp"
#include "pba/gl_types.hpp"

namespace ds_pba
{

GLMesh create_cube_mesh();
GLMesh create_grid_mesh(int n_lines_per_side, f32 spacing, f32 axis_alpha, f32 minor_alpha);
GLMesh create_sphere_mesh(int n_lat, int n_lon, f32 radius);

}  // namespace ds_pba
