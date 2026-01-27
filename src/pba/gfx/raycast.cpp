// pba/gfx/raycast.cpp
#include "glm/geometric.hpp"
#include "pba/core/geometry.hpp"
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/core/math_types.hpp"
#include "pba/engine/scene_context.hpp"
#include "pba/gfx/raycast.hpp"

#include <gsl/assert>

namespace ds_pba
{
[[nodiscard]] std::optional<Raycast>
raycast(const SceneContext& scene_context, const Ray& ray) noexcept
{
    Expects(is_normalized(ray.dir));
    f32 best_t{1e30f};

    bool found{false};
    u32 best_object_id{};
    ObjectType best_type{};

    // Forwarding reference to intersection function, see
    // https://www.scs.stanford.edu/~dm/blog/param-pack.html
    auto check_hits = [&](const auto& objects, ObjectType type, auto&& intersect_fn) -> void
    {
        for (const Object& o : objects)
        {
            if (auto res = intersect_fn(ray, o.transform.model_matrix()))
            {
                const f32 t{*res};
                if (t < best_t)
                {
                    best_t = t;
                    best_type = type;
                    best_object_id = o.id;
                    found = true;
                }
            }
        }
    };

    check_hits(scene_context.cube_objects, ObjectType::Cube, intersect_ray_cube);
    check_hits(scene_context.sphere_objects, ObjectType::Sphere, intersect_ray_sphere);
    check_hits(scene_context.marble_bust_objects, ObjectType::MarbleBust, intersect_ray_sphere);

    if (!found)
    {
        return std::nullopt;
    }

    assert(best_t > 0.0f);

    return Raycast{
        .ray = ray,
        .hit = ray.origin + best_t * ray.dir,
        .t = best_t,
        .object_id = best_object_id,
        .object_type = best_type,
    };
}

[[nodiscard]] Ray ray_from_imgui_rect(
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

    const ClipToWorldMatrix c2w = clip_to_world(camera_proj_matrix, camera_view_matrix);

    const Pos3 near_w = c2w.unproject_ndc(x_ndc, y_ndc, -1.0f);
    const Pos3 far_w = c2w.unproject_ndc(x_ndc, y_ndc, +1.0f);

    return Ray{
        .origin = near_w,
        .dir = glm::normalize(Dir3{far_w - near_w}),
    };
}

/// https://pbr-book.org/4ed/Shapes/Spheres
[[nodiscard]] std::optional<f32>
intersect_ray_sphere(const Ray& ray, const ModelMatrix& model_matrix) noexcept
{
    Expects(is_normalized(ray.dir));

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
[[nodiscard]] std::optional<f32> intersect_ray_ground(const Ray& ray) noexcept
{
    Expects(is_normalized(ray.dir));
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

/// Using the Slab method, see for example
/// https://www.pbr-book.org/4ed/Shapes/Basic_Shape_Interface
[[nodiscard]] std::optional<f32>
intersect_ray_cube(const Ray& ray_world, const ModelMatrix& model_matrix) noexcept
{
    Expects(is_normalized(ray_world.dir));

    f32 t_min{-1e30f};
    f32 t_max{+1e30f};

    auto slab = [&](f32 o, f32 d, f32 mn, f32 mx) -> bool
    {
        if (std::abs(d) < 1e-8f)
        {
            return (o >= mn && o <= mx);
        }

        f32 t1{(mn - o) / d};
        f32 t2{(mx - o) / d};
        if (t1 > t2)
        {
            std::swap(t1, t2);
        }

        t_min = std::max(t_min, t1);
        t_max = std::min(t_max, t2);
        return t_min <= t_max;
    };

    const auto world_to_model = model_matrix.world_to_model();

    const Pos3 origin_local = world_to_model.transform_position(ray_world.origin);
    const Dir3 dir_local = world_to_model.transform_direction(ray_world.dir);

    constexpr Pos3 bmin{-0.5f, -0.5f, -0.5f};
    constexpr Pos3 bmax{+0.5f, +0.5f, +0.5f};

    // clang-format off
    if (!slab(origin_local.x, dir_local.x, bmin.x, bmax.x)) return std::nullopt;
    if (!slab(origin_local.y, dir_local.y, bmin.y, bmax.y)) return std::nullopt;
    if (!slab(origin_local.z, dir_local.z, bmin.z, bmax.z)) return std::nullopt;
    // clang-format on

    const f32 t_local = (t_min > 0.0f) ? t_min : ((t_max > 0.0f) ? t_max : -1.0f);
    if (t_local <= 0.0f)
    {
        return std::nullopt;
    }

    const Pos3 hit_local = origin_local + t_local * dir_local;
    const Pos3 hit_world = model_matrix.transform_position(hit_local);

    // Assumes ray_world.dir is unit-length (checked above).
    const f32 t_world = glm::dot(hit_world - ray_world.origin, ray_world.dir);
    if (t_world <= 0.0f)
    {
        return std::nullopt;
    }

    return t_world;
}

}  // namespace ds_pba
