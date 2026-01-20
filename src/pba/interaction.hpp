// pba/picking.hpp
#pragma once

#include "pba/types.hpp"  // IWYU pragma: keep

struct GLFWwindow;

namespace ds_pba
{
Ray ray_from_mouse(
    GLFWwindow* window,
    f64 mouse_x,
    f64 mouse_y,
    const glm::mat4& camera_view_matrix,
    const glm::mat4& camera_proj_matrix
);

Ray ray_from_imgui_rect(
    const glm::vec2& mouse_pos,
    const glm::vec2& rect_pos,
    const glm::vec2& rect_size,
    const glm::mat4& camera_view_matrix,
    const glm::mat4& camera_proj_matrix
);

std::optional<f32> intersect_ray_cube(const Ray& ray, const glm::mat4& model);
std::optional<f32> intersect_ray_sphere(const Ray& ray, const glm::mat4& model);

}  // namespace ds_pba
