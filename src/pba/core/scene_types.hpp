// pba/core/scene_types.hpp
#pragma once

#include "pba/core/core_types.hpp"

namespace ds_pba
{
enum class SceneId : u32
{
    AttractorsAndRepulsivePivot = 0,

    SmallPyramid_Projectiles_NoGround_Gravity,
    AttractorToOrigin_NoGravity,
    AttractorToOrigin_WithGravity,
    LargePyramid15_Ground_Gravity,
    Pyramid3D_HeavyCubeDrop,
    Motors_Elongated_NoGravity,
    NBody_SunAnd3Planets,
    NBody_ThreeBodyEqual,
    MovingAttractor_TargetMovesInCircle,
    OscillatingUniformForce_WithInternalTime,

    InclinedPlane_SlidingCubes,
    BoxDrop_Container,
    ProjectileWall,

    Count
};
}
