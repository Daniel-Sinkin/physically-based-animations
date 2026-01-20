// pba/camera.hpp
#pragma once

#include "pba/types.hpp"  // IWYU pragma: keep

namespace ds_pba
{

struct Camera
{
    glm::vec3 pivot{0.0f, 0.0f, 0.0f};
    f32 distance = 8.0f;

    f32 yaw = glm::radians(45.0f);
    f32 pitch = glm::radians(25.0f);

    f32 fov_y = glm::radians(45.0f);
    f32 z_near = 0.1f;
    f32 z_far = 250.0f;

    [[nodiscard]] glm::vec3 position() const;
    [[nodiscard]] glm::mat4 view_matrix() const;
    [[nodiscard]] glm::mat4 proj_matrix(f32 aspect) const;
};

}  // namespace ds_pba