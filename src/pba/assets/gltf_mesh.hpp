// pba/assets/gltf_mesh.hpp
#pragma once

#include "pba/assets/mesh_data.hpp"
#include "pba/engine/scene_types.hpp"

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

std::expected<MeshDataPN, GltfLoadError>
load_gltf_mesh(const std::string& path, const Transform& preprocess);

std::expected<MeshDataPN, GltfLoadError> load_model_mesh(const std::string& model_name);
struct AccessorView
{
    const std::byte* base{};
    usize stride{};
    usize count{};
};

}  // namespace ds_pba
