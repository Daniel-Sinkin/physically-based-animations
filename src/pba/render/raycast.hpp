// pba/raycast.hpp
#pragma once

#include "pba/core_types.hpp"
#include "pba/math_types.hpp"
#include "pba/scene_types.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <optional>

struct GLFWwindow;

namespace ds_pba
{
struct SceneContext;

struct Ray
{
    Position3 origin{};
    Direction3 dir{};

    [[nodiscard]] bool valid() const
    {  // Has to be normalised
        return std::abs(glm::length(dir) - 1.0f) < 0.0001f;
    }
};

struct Raycast
{
    Ray ray;
    Position3 hit;
    f32 t;
    ObjectId object_id;
    ObjectType object_type;
};
Ray ray_from_mouse(
    GLFWwindow* window,
    f64 mouse_x,
    f64 mouse_y,
    const ViewMatrix& camera_view_matrix,
    const ProjMatrix& camera_proj_matrix
);

Ray ray_from_imgui_rect(
    const glm::vec2& mouse_pos,
    const glm::vec2& rect_pos,
    const glm::vec2& rect_size,
    const ViewMatrix& camera_view_matrix,
    const ProjMatrix& camera_proj_matrix
);

std::optional<f32> intersect_ray_cube(const Ray& ray, const ModelMatrix& model);
std::optional<f32> intersect_ray_sphere(const Ray& ray, const ModelMatrix& model);

std::optional<Raycast> raycast(const SceneContext& context, const Ray& ray);
}  // namespace ds_pba
