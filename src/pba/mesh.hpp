// pba/mesh.hpp
#pragma once

#include "pba/core_types.hpp"
#include "pba/gl_types.hpp"
#include "pba/render_context.hpp"

namespace ds_pba
{
GLMesh create_cube_mesh();
GLMesh create_cylinder_mesh(int n_segments, f32 radius, f32 height);
GLMesh create_grid_mesh(GridSettings grid);
GLMesh create_pyramid_mesh();
GLMesh create_quad_mesh();
GLMesh create_sphere_mesh(int n_lat, int n_lon, f32 radius);
}  // namespace ds_pba
