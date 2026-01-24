// pba/camera.hpp
#pragma once

#include "pba/constants.hpp"
#include "pba/core_types.hpp"
#include "pba/math_types.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ds_pba
{

struct Camera
{
    Position3 pivot{k_camera_pivot};
    f32 distance{k_camera_distance};

    f32 yaw{glm::radians(k_camera_yaw)};
    f32 pitch{glm::radians(k_camera_pitch)};

    f32 fov_y{glm::radians(k_camera_fov_y)};
    f32 z_near{k_camera_z_near};
    f32 z_far{k_camera_z_far};

    [[nodiscard]] Position3 position() const;
    [[nodiscard]] ViewMatrix view_matrix() const;
    [[nodiscard]] ProjMatrix proj_matrix(f32 aspect) const;

    [[nodiscard]] Direction3 right() const;
    [[nodiscard]] Direction3 up() const;
};

}  // namespace ds_pba
