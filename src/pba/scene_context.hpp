// pba/scene_context.hpp
#pragma once

#include "camera.hpp"
#include "glm/fwd.hpp"
#include "pba/scene_types.hpp"
#include "pba/types.hpp"  // IWYU pragma: keep

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace ds_pba
{
namespace fs = std::filesystem;

struct SceneContext
{
    Camera camera{};
    std::vector<Object> cube_objects{};
    std::vector<Object> sphere_objects{};
    std::optional<usize> selected_index{};
    std::optional<ObjectType> selected_type{};
};

}  // namespace ds_pba
