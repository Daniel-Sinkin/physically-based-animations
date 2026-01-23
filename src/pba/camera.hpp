// pba/camera.hpp
#pragma once

#include "pba/core_types.hpp"
#include "pba/math_types.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ds_pba
{

struct Camera
{
    Position3 pivot{0.0f, 0.0f, 0.0f};
    f32 distance{8.0f};

    f32 yaw{glm::radians(45.0f)};
    f32 pitch{glm::radians(25.0f)};

    f32 fov_y{glm::radians(45.0f)};
    f32 z_near{0.1f};
    f32 z_far{250.0f};

    [[nodiscard]] Position3 position() const;
    [[nodiscard]] ViewMatrix view_matrix() const;
    [[nodiscard]] ProjMatrix proj_matrix(f32 aspect) const;

    [[nodiscard]] Direction3 right() const;
    [[nodiscard]] Direction3 up() const;
};

}  // namespace ds_pba
