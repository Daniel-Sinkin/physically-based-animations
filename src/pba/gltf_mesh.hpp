// pba/gltf_mesh.hpp
#pragma once

#include "pba/gl_types.hpp"

#include <expected>
#include <string>

namespace ds_pba
{

enum class GltfLoadError
{
    ParseError,
    UnsupportedPrimitive,
    MissingPosition,
    UnsupportedAccessorType,
};
enum class AxisFix
{
    None,

    RotX90,
    RotX180,
    RotX270,

    RotY90,
    RotY180,
    RotY270,

    RotZ90,
    RotZ180,
    RotZ270,

    RotX90_Z90,
    RotX90_Z180,
    RotX90_Z270,

    RotX180_Z90,
    RotX180_Z270,

    RotX270_Z90,
    RotX270_Z180,
    RotX270_Z270,

    RotY90_Z180,
    RotY270_Z180,

    RotZ90_X90,
    RotZ90_X270,
    RotZ270_X90,
    RotZ270_X270,

    FlipX,
    FlipY,
    FlipZ,
};

std::expected<GLMesh, GltfLoadError> load_gltf_mesh(const std::string& path, AxisFix);

}  // namespace ds_pba
