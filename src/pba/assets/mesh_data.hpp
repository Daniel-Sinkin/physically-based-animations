// pba/assets/mesh_data.hpp
#pragma once

#include "pba/core/core_types.hpp"

#include <vector>

namespace ds_pba
{
// P == Position
// N == Normal
// T == TexCoord (UV)

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

struct MeshV_PNT
{
    f32 px, py, pz;
    f32 nx, ny, nz;
    f32 u, v;
};
static_assert(sizeof(MeshV_PNT) == 8 * sizeof(f32));
static_assert(offsetof(MeshV_PNT, nx) == 3 * sizeof(f32));
static_assert(offsetof(MeshV_PNT, u) == 6 * sizeof(f32));

struct MeshDataPNT
{
    std::vector<MeshV_PNT> vertices{};
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
