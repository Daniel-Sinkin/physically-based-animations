// pba/assets/mesh.hpp
#pragma once

#include "pba/assets/mesh_data.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/render_types.hpp"

namespace ds_pba
{
[[nodiscard]] auto create_cube_mesh() -> MeshDataPN;
[[nodiscard]] auto create_quad_mesh() -> MeshDataPN;
[[nodiscard]] auto create_pyramid_mesh() -> MeshDataPN;
[[nodiscard]] auto create_cylinder_mesh(int n_segments, f32 radius, f32 height) -> MeshDataPN;
[[nodiscard]] auto create_sphere_mesh(int n_lat, int n_lon, f32 radius) -> MeshDataPN;
[[nodiscard]] auto create_grid_mesh(GridSettings grid) -> MeshDataPColor;
}  // namespace ds_pba
