// pba/scene/camera.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/geometry.hpp"
#include "pba/core/math_types.hpp"
//
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ds_pba
{

struct Camera
{
    Pos3 pivot{k_camera_pivot};
    f32 distance{k_camera_distance};

    f32 yaw{glm::radians(k_camera_yaw)};
    f32 pitch{glm::radians(k_camera_pitch)};

    f32 fov_y{glm::radians(k_camera_fov_y)};
    f32 z_near{k_camera_z_near};
    f32 z_far{k_camera_z_far};

    [[nodiscard]] Pos3 position() const noexcept;
    [[nodiscard]] ViewMatrix view_matrix() const noexcept;
    [[nodiscard]] ProjMatrix proj_matrix(f32 aspect) const noexcept;

    [[nodiscard]] f32 view_height_world() const noexcept;

    [[nodiscard]] f32 units_per_pixel_y(f32 viewport_height_px) const noexcept;

    [[nodiscard]] Dir3
    pan_offset_world(f32 dx_px, f32 dy_px, f32 viewport_height_px) const noexcept;

    [[nodiscard]] Dir3 right() const noexcept;
    [[nodiscard]] Dir3 up() const noexcept;
};

}  // namespace ds_pba
