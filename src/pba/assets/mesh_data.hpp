// pba/mesh_data.hpp
#pragma once

#include "pba/core_types.hpp"

#include <vector>

namespace ds_pba
{

struct MeshV_PN
{
    f32 px, py, pz;
    f32 nx, ny, nz;
};
static_assert(sizeof(MeshV_PN) == 6 * sizeof(f32));
static_assert(offsetof(MeshV_PN, nx) == 3 * sizeof(f32));

struct MeshDataPN
{
    std::vector<MeshV_PN> vertices{};
    std::vector<u32> indices{};
};

struct MeshV_PColor
{
    f32 px, py, pz;
    f32 r, g, b, a;
};
static_assert(sizeof(MeshV_PColor) == 7 * sizeof(f32));
static_assert(offsetof(MeshV_PColor, r) == 3 * sizeof(f32));

struct MeshDataPColor
{
    std::vector<MeshV_PColor> vertices{};
    std::vector<u32> indices{};
};

}  // namespace ds_pba
