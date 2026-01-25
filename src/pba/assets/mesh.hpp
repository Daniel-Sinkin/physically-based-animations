// pba/assets/mesh.hpp
#pragma once

#include "pba/assets/mesh_data.hpp"
#include "pba/core/core_types.hpp"
#include "pba/render/render_context.hpp"

#include <optional>

namespace ds_pba
{
[[nodiscard]] MeshDataPN create_cube_mesh();
[[nodiscard]] MeshDataPN create_quad_mesh();
[[nodiscard]] MeshDataPN create_pyramid_mesh();

[[nodiscard]] std::optional<MeshDataPN>
create_cylinder_mesh(int n_segments, f32 radius, f32 height);
[[nodiscard]] std::optional<MeshDataPN> create_sphere_mesh(int n_lat, int n_lon, f32 radius);

[[nodiscard]] std::optional<MeshDataPColor> create_grid_mesh(GridSettings grid);
}  // namespace ds_pba
