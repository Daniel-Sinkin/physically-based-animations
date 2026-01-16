// pba/camera.cpp
#include "camera.hpp"

#include <cmath>

namespace ds_pba {

glm::vec3 Camera::position() const {
    const f32 cos_pitch = std::cos(pitch);
    const f32 sin_pitch = std::sin(pitch);
    const f32 cos_yaw   = std::cos(yaw);
    const f32 sin_yaw   = std::sin(yaw);

    const glm::vec3 offset{
        distance * cos_pitch * cos_yaw,
        distance * cos_pitch * sin_yaw,
        distance * sin_pitch,
    };

    return pivot + offset;
}

glm::mat4 Camera::view_matrix() const {
    return glm::lookAt(position(), pivot, glm::vec3(0, 0, 1));
}

glm::mat4 Camera::proj_matrix(f32 aspect) const {
    return glm::perspective(fov_y, aspect, z_near, z_far);
}

} // namespace ds_pba