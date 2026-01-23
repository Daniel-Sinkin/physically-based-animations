// mesh_data.hpp
#pragma once
#include "core_types.hpp"

namespace ds_pba
{
struct MeshV
{
    f32 px, py, pz;
    f32 nx, ny, nz;
};
static_assert(sizeof(MeshV) == 6 * sizeof(f32));
static_assert(offsetof(MeshV, nx) == 3 * sizeof(f32));
struct MeshData
{

    std::vector<MeshV> vertices{};
    std::vector<u32> indices{};
};
}  // namespace ds_pba
