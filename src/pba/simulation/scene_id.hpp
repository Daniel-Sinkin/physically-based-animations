// pba/simulation/scene_id.hpp
#pragma once

#include "pba/core/core_types.hpp"

namespace ds_pba
{
enum class SceneId : u32
{
    StablePyramid2D3D = 0,
    ProjectileWall,
    CubeCloud1200,
    OrbitalRotorVortex,
    DominoSpiralCascade,
    TumblerDrum,
    CupRainCollapse,
    TowerDemolition,
    TriplePyramidSiege,
    CubeCrossfireArena,
    NBodyCubeGalaxy,
    InclinedAvalanche,

    Count
};
inline constexpr SceneId k_default_scene = SceneId::StablePyramid2D3D;

}  // namespace ds_pba
