// pba/assets/gltf_mesh.hpp
#pragma once

#include "pba/assets/mesh_data.hpp"
#include "pba/scene/world_types.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace ds_pba
{

struct AccessorView
{
    const std::byte* base{};
    usize stride{};
    usize count{};
};

[[nodiscard]] std::optional<MeshDataPN>
load_gltf_mesh(const std::string& path, const Transform& preprocess);

[[nodiscard]] std::optional<MeshDataPN> load_model_mesh(std::string_view model_name);

// Textured mesh variant (POSITION/NORMAL/TEXCOORD_0)
[[nodiscard]] std::optional<MeshDataPNT>
load_gltf_mesh_pnt(const std::string& path, const Transform& preprocess);

[[nodiscard]] std::optional<MeshDataPNT> load_model_mesh_pnt(std::string_view model_name);

}  // namespace ds_pba
