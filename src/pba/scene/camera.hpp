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

    [[nodiscard]] auto position() const noexcept -> Pos3;
    [[nodiscard]] auto view_matrix() const noexcept -> ViewMatrix;
    [[nodiscard]] auto proj_matrix(f32 aspect) const noexcept -> ProjMatrix;

    [[nodiscard]] auto view_height_world() const noexcept -> f32;
    [[nodiscard]] auto units_per_pixel_y(f32 viewport_height_px) const noexcept -> f32;

    [[nodiscard]] auto pan_offset_world(f32 dx_px, f32 dy_px, f32 viewport_height_px) const noexcept
        -> Dir3;

    [[nodiscard]] auto right() const noexcept -> Dir3;
    [[nodiscard]] auto up() const noexcept -> Dir3;
};

}  // namespace ds_pba
