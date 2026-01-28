// pba/gfx/raycast.cpp
#include "glm/geometric.hpp"
#include "pba/core/core_types.hpp"
#include "pba/core/geometry.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/core/math_types.hpp"
#include "pba/gfx/raycast.hpp"
#include "pba/scene/entity.hpp"
#include "pba/scene/world.hpp"

#include <gsl/assert>

namespace ds_pba
{
std::optional<Raycast> raycast(const World& world, const Ray& ray) noexcept
{
    Expects(ray.valid());
    auto best_t = k_f32_max;

    u32 best_object_id{};

    // Forwarding reference to intersection function, see
    // https://www.scs.stanford.edu/~dm/blog/param-pack.html
    auto check_hit = [&](const auto& entity, auto&& intersect_fn) -> void
    {
        if (auto res = intersect_fn(ray, entity.transform.model_matrix()); res && *res < best_t)
        {
            best_t = *res;
            best_object_id = entity.id;
        }
    };
    for (const auto& entity : world.entities())
    {
        switch (entity.type)
        {
            case EntityType::Cube:
                check_hit(entity, intersect_ray_cube);
                break;
        }
    }

    if (best_t == k_f32_max)
    {
        return std::nullopt;
    }

    assert(best_t > 0.0f);

    return Raycast{
        .ray = ray,
        .hit = ray.origin + best_t * ray.dir,
        .t = best_t,
        .object_id = best_object_id,
    };
}

Ray ray_from_imgui_rect(
    const glm::vec2& mouse_pos,
    const glm::vec2& rect_pos,
    const glm::vec2& rect_size,
    const ViewMatrix& camera_view_matrix,
    const ProjMatrix& camera_proj_matrix
) noexcept
{
    const auto lx = mouse_pos.x - rect_pos.x;
    const auto ly = mouse_pos.y - rect_pos.y;

    const auto w = std::max(1.0f, rect_size.x);
    const auto h = std::max(1.0f, rect_size.y);

    const auto x_ndc = 2.0f * (lx / w) - 1.0f;
    const auto y_ndc = 1.0f - 2.0f * (ly / h);

    const auto c2w = clip_to_world(camera_proj_matrix, camera_view_matrix);
    const Pos3 near_w = c2w.unproject_ndc(x_ndc, y_ndc, -1.0f);
    const Pos3 far_w = c2w.unproject_ndc(x_ndc, y_ndc, +1.0f);

    return Ray{
        .origin = near_w,
        .dir = glm::normalize(Dir3{far_w - near_w}),
    };
}

/// https://pbr-book.org/4ed/Shapes/Spheres
std::optional<f32> intersect_ray_sphere(const Ray& ray, const ModelMatrix& model_matrix) noexcept
{
    Expects(ray.valid());

    const auto world_to_model = model_matrix.world_to_model();

    const Pos3 origin_local = world_to_model.transform_position(ray.origin);
    const Dir3 dir_local = glm::normalize(world_to_model.transform_direction(ray.dir));

    const auto a = 1.0f;
    const auto b = 2.0f * glm::dot(origin_local, dir_local);
    const auto c = glm::dot(origin_local, origin_local) - 1.0f;
    const auto disc = b * b - 4.0f * a * c;  // a == 1.0f
    if (disc < 0.0f)
    {
        return std::nullopt;
    }

    const auto s = std::sqrt(disc);
    const auto inv2a = 0.5f / a;
    const auto t0 = (-b - s) * inv2a;
    const auto t1 = (-b + s) * inv2a;

    const auto t_local = (t0 > 0.0f) ? t0 : ((t1 > 0.0f) ? t1 : -1.0f);
    if (t_local <= 0.0f)
    {
        return std::nullopt;
    }

    const Pos3 hit_local{origin_local + t_local * dir_local};
    const Pos3 hit_world{model_matrix.transform_position(hit_local)};

    const f32 t_world{glm::dot(hit_world - ray.origin, ray.dir)};
    if (t_world <= 0.0f)
    {
        return std::nullopt;
    }
    return t_world;
}

/// Solve ray.origin.z + t * ray.dir.z = 0 for t
std::optional<f32> intersect_ray_ground(const Ray& ray) noexcept
{
    Expects(ray.valid());
    if (std::abs(ray.origin.z) < 1e-5f)
    {
        return 0.0f;
    }

    if (std::abs(ray.dir.z) < 1e-5f)
    {
        return std::nullopt;
    }
    return -ray.origin.z / ray.dir.z;
}

// Using the Slab method, see for example
// https://www.pbr-book.org/4ed/Shapes/Basic_Shape_Interface
std::optional<f32> intersect_ray_cube(const Ray& ray, const ModelMatrix& model_matrix) noexcept
{
    Expects(ray.valid());

    auto t_enter = k_f32_min;
    auto t_exit = k_f32_max;

    auto slab = [&](f32 o, f32 d, f32 mn, f32 mx) -> bool
    {
        if (std::abs(d) < 1e-8f)
        {
            return (o >= mn && o <= mx);
        }
        auto t_axis_enter = (mn - o) / d;
        auto t_axis_exit = (mx - o) / d;
        if (t_axis_enter > t_axis_exit)
        {
            std::swap(t_axis_enter, t_axis_exit);
        }
        t_enter = std::max(t_enter, t_axis_enter);
        t_exit = std::min(t_exit, t_axis_exit);
        return (t_enter <= t_exit);
    };

    const auto world_to_model = model_matrix.world_to_model();

    const Pos3 origin_local = world_to_model.transform_position(ray.origin);
    const Dir3 dir_local = world_to_model.transform_direction(ray.dir);

    constexpr auto local_box = AABB::unit();
    if (!slab(origin_local.x, dir_local.x, local_box.min.x, local_box.max.x)
        || !slab(origin_local.y, dir_local.y, local_box.min.y, local_box.max.y)
        || !slab(origin_local.z, dir_local.z, local_box.min.z, local_box.max.z))
    {
        return std::nullopt;
    }

    const auto t_local = (t_enter > 0.0f) ? t_enter : ((t_exit > 0.0f) ? t_exit : -1.0f);
    if (t_local <= 0.0f)
    {
        return std::nullopt;
    }

    const Pos3 hit_local = origin_local + t_local * dir_local;
    const Pos3 hit_world = model_matrix.transform_position(hit_local);

    const auto t_world = glm::dot(hit_world - ray.origin, ray.dir);
    if (t_world <= 0.0f)
    {
        return std::nullopt;
    }

    return t_world;
}

}  // namespace ds_pba
