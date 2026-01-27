// pba/gfx/camera.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/core/geometry.hpp"
#include "pba/gfx/camera.hpp"
//
#include "pba/core/math_types.hpp"

namespace ds_pba
{

[[nodiscard]] Pos3 Camera::position() const noexcept
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

[[nodiscard]] ViewMatrix Camera::view_matrix() const noexcept
{
    return ViewMatrix{glm::lookAt(position(), pivot, k_axis_z)};
}

[[nodiscard]] ProjMatrix Camera::proj_matrix(f32 aspect) const noexcept
{
    return ProjMatrix{glm::perspective(fov_y, aspect, z_near, z_far)};
}

[[nodiscard]] Dir3 Camera::right() const noexcept
{
    const Pos3 pos{position()};
    const Dir3 forward{glm::normalize(pivot - pos)};
    const Dir3 world_up{0.0f, 0.0f, 1.0f};
    const Dir3 right{glm::normalize(glm::cross(forward, world_up))};
    return right;
}

[[nodiscard]] Dir3 Camera::up() const noexcept
{
    const glm::vec3 pos{position()};
    const Dir3 forward{glm::normalize(pivot - pos)};
    const Dir3 up{glm::normalize(glm::cross(right(), forward))};
    return up;
}

}  // namespace ds_pba
