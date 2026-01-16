// pba/picking.hpp
#pragma once

#include "types.hpp"

struct GLFWwindow;

namespace ds_pba {

f32 max3(f32 a, f32 b, f32 c);

bool intersect_sphere(const Ray &ray, const glm::vec3 &center, f32 radius, f32 &t_hit);

Ray ray_from_mouse(
    GLFWwindow *window,
    f64 mouse_x,
    f64 mouse_y,
    const glm::mat4 &camera_view_matrix,
    const glm::mat4 &camera_proj_matrix);

Ray ray_from_mouse_in_rect(
    GLFWwindow* window,
    f64 mouse_x,
    f64 mouse_y,
    int rect_x,
    int rect_y,
    int rect_w,
    int rect_h,
    const glm::mat4& camera_view_matrix,
    const glm::mat4& camera_proj_matrix);

bool intersect_unit_cube_obb(
    const Ray &ray_world,
    const glm::mat4 &model,
    f32 &t_world_out);

} // namespace ds_pba