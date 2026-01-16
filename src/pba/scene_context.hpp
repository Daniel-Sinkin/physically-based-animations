
// pba/main.cpp
#pragma once
#include "pba/types.hpp"
#include "pba/camera.hpp"

#include <cstdlib>
#include <optional>
#include <vector>

namespace ds_pba {
struct SceneContext {
    Camera camera{};
    std::vector<Object> cube_objects{};
    std::optional<usize> selected_index{};
};
} // namespace ds_pba