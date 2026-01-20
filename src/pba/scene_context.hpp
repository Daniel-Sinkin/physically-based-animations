// pba/scene_context.hpp
#pragma once

#include "pba/camera.hpp"
#include "pba/core_types.hpp"
#include "pba/scene_types.hpp"

#include <optional>
#include <unordered_map>
#include <vector>

namespace ds_pba
{
struct SceneContext
{
    Camera camera{};
    std::vector<Object> cube_objects{};
    std::vector<Object> sphere_objects{};
    std::optional<usize> selected_index{};
    std::optional<ObjectType> selected_type{};
    std::unordered_map<ObjectId, Object*> object_map{};

    void setup();
};

}  // namespace ds_pba
