// pba/scene/camera.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/core/geometry.hpp"
#include "pba/scene/camera.hpp"
//
#include "pba/core/math_types.hpp"

namespace ds_pba
{

Pos3 Camera::position() const noexcept
{
    const f32 cos_pitch{std::cos(pitch)};
    const f32 sin_pitch{std::sin(pitch)};
    const f32 cos_yaw{std::cos(yaw)};
    const f32 sin_yaw{std::sin(yaw)};

    const glm::vec3 offset{
        distance * cos_pitch * cos_yaw,
        distance * cos_pitch * sin_yaw,
        distance * sin_pitch,
    };

    return pivot + offset;
}

ViewMatrix Camera::view_matrix() const noexcept
{
    return ViewMatrix{glm::lookAt(position(), pivot, k_axis_z)};
}

ProjMatrix Camera::proj_matrix(f32 aspect) const noexcept
{
    return ProjMatrix{glm::perspective(fov_y, aspect, z_near, z_far)};
}

Dir3 Camera::right() const noexcept
{
    const Pos3 pos{position()};
    const Dir3 forward{glm::normalize(pivot - pos)};
    const Dir3 world_up{0.0f, 0.0f, 1.0f};
    const Dir3 right{glm::normalize(glm::cross(forward, world_up))};
    return right;
}

Dir3 Camera::up() const noexcept
{
    const glm::vec3 pos{position()};
    const Dir3 forward{glm::normalize(pivot - pos)};
    const Dir3 up{glm::normalize(glm::cross(right(), forward))};
    return up;
}

f32 Camera::view_height_world() const noexcept
{
    const auto half_fov_y_rad = 0.5f * fov_y;
    const auto view_half_height_world = distance * std::tan(half_fov_y_rad);
    return 2.0f * view_half_height_world;
}

f32 Camera::units_per_pixel_y(f32 viewport_height_px) const noexcept
{
    const auto clamped_height_px = std::max(1.0f, viewport_height_px);
    return view_height_world() / clamped_height_px;
}

Dir3 Camera::pan_offset_world(f32 dx_px, f32 dy_px, f32 viewport_height_px) const noexcept
{
    const auto units_per_px_y = units_per_pixel_y(viewport_height_px);

    const auto right_offset_world = (-dx_px * units_per_px_y) * right();
    const auto up_offset_world = (dy_px * units_per_px_y) * up();

    return right_offset_world + up_offset_world;
}

}  // namespace ds_pba
