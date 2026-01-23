// pba/mesh.hpp
#pragma once

#include "pba/core_types.hpp"
#include "pba/gl_types.hpp"
#include "pba/mesh_data.hpp"
#include "pba/render_context.hpp"

namespace ds_pba
{
[[nodiscard]] MeshData create_cube_mesh();
[[nodiscard]] MeshData create_cylinder_mesh(int n_segments, f32 radius, f32 height);
[[nodiscard]] MeshData create_pyramid_mesh();
[[nodiscard]] MeshData create_quad_mesh();
[[nodiscard]] MeshData create_sphere_mesh(int n_lat, int n_lon, f32 radius);
GLMesh create_grid_mesh(GridSettings grid);  // TODO: Make this also GL independent
}  // namespace ds_pba
