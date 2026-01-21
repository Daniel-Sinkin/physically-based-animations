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

std::expected<GLMesh, GltfLoadError> load_gltf_mesh(const std::string& path);

}  // namespace ds_pba
