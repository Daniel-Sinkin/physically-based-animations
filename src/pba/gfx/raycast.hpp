// pba/gfx/raycast.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/geometry.hpp"
#include "pba/core/math_types.hpp"
#include "pba/scene/entity_id.hpp"
//
#include <optional>
//
#include <glm/glm.hpp>

struct GLFWwindow;

namespace ds_pba
{
class World;

struct Ray
{
    Pos3 origin{};
    Dir3 dir{};

    [[nodiscard]] bool valid() const noexcept
    {  // Has to be normalised
        return is_normalized(dir);
    }
};

struct Raycast
{
    Ray ray;
    Pos3 hit;
    f32 t;
    EntityId object_id;
};
[[nodiscard]] Ray ray_from_imgui_rect(
    const glm::vec2& mouse_pos,
    const glm::vec2& rect_pos,
    const glm::vec2& rect_size,
    const ViewMatrix& camera_view_matrix,
    const ProjMatrix& camera_proj_matrix
) noexcept;

[[nodiscard]] std::optional<f32>
intersect_ray_cube(const Ray& ray, const ModelMatrix& model) noexcept;
[[nodiscard]] std::optional<f32>
intersect_ray_sphere(const Ray& ray, const ModelMatrix& model) noexcept;

[[nodiscard]] std::optional<Raycast> raycast(const World& context, const Ray& ray) noexcept;
}  // namespace ds_pba
