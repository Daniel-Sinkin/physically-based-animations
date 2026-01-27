// pba/gfx/raycast.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/core/math_types.hpp"
#include "pba/engine/scene_context.hpp"
#include "pba/gfx/raycast.hpp"

namespace ds_pba
{
[[nodiscard]] std::optional<Raycast>
raycast(const SceneContext& scene_context, const Ray& ray) noexcept
{
    f32 best_t{1e30f};

    std::optional<usize> best_idx{};
    std::optional<u32> best_object_id{};
    std::optional<ObjectType> best_type{};

    for (usize i{0zu}; i < scene_context.cube_objects.size(); ++i)
    {
        const Object& o{scene_context.cube_objects[i]};
        if (auto res = intersect_ray_cube(ray, o.transform.model_matrix()))
        {
            const f32 t{*res};
            if (t < best_t)
            {
                best_t = t;
                best_idx = i;
                best_type = ObjectType::Cube;
                best_object_id = o.id;
            }
        }
    }
    for (usize i{0zu}; i < scene_context.sphere_objects.size(); ++i)
    {
        const Object& o{scene_context.sphere_objects[i]};
        const ModelMatrix M{o.transform.model_matrix()};

        if (auto res = intersect_ray_sphere(ray, M))
        {
            const f32 t{*res};
            if (t < best_t)
            {
                best_t = t;
                best_idx = i;
                best_type = ObjectType::Sphere;
                best_object_id = o.id;
            }
        }
    }
    for (usize i{0zu}; i < scene_context.marble_bust_objects.size(); ++i)
    {
        const Object& o{scene_context.marble_bust_objects[i]};
        const ModelMatrix M{o.transform.model_matrix()};

        if (auto res = intersect_ray_sphere(ray, M))
        {
            const f32 t{*res};
            if (t < best_t)
            {
                best_t = t;
                best_idx = i;
                best_type = ObjectType::MarbleBust;
                best_object_id = o.id;
            }
        }
    }

    if (!best_idx)
    {
        return std::nullopt;
    }
    assert(best_t > 0.0f);
    assert(best_type);

    return Raycast{
        .ray = ray,
        .hit = ray.origin + best_t * ray.dir,
        .t = best_t,
        .object_id = *best_object_id,
        .object_type = *best_type
    };
}

[[nodiscard]] Ray ray_from_mouse(
    not_null<GLFWwindow*> window,
    f64 mouse_x,
    f64 mouse_y,
    const ViewMatrix& camera_view_matrix,
    const ProjMatrix& camera_proj_matrix
) noexcept
{
    int ww{0}, wh{0};
    glfwGetWindowSize(window, &ww, &wh);
    int fbw{0}, fbh{0};
    glfwGetFramebufferSize(window, &fbw, &fbh);

    const auto window_width_f = narrow_cast<f32>(ww);
    const auto window_height_f = narrow_cast<f32>(wh);
    const auto fb_width_f = narrow_cast<f32>(fbw);
    const auto fb_height_f = narrow_cast<f32>(fbh);

    const auto scale_x = (window_width_f > 0) ? (fb_width_f / window_width_f) : 1.0f;
    const auto scale_y = (window_height_f > 0) ? (fb_height_f / window_height_f) : 1.0f;

    const auto mx = narrow_cast<f32>(mouse_x) * scale_x;
    const auto my = narrow_cast<f32>(mouse_y) * scale_y;

    const auto x_ndc = (fb_width_f > 0) ? (2.0f * mx / fb_width_f - 1.0f) : 0.0f;
    const auto y_ndc = (fb_height_f > 0) ? (1.0f - 2.0f * my / fb_height_f) : 0.0f;

    const auto invPV = glm::inverse(camera_proj_matrix * camera_view_matrix);

    const glm::vec4 near_ndc{x_ndc, y_ndc, -1.0f, 1.0f};
    const glm::vec4 far_ndc{x_ndc, y_ndc, 1.0f, 1.0f};

    auto near_w = invPV * near_ndc;
    auto far_w = invPV * far_ndc;
    near_w /= near_w.w;
    far_w /= far_w.w;

    return Ray{
        .origin = glm::vec3(near_w),
        .dir = glm::normalize(glm::vec3(far_w - near_w)),
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

    const auto invPV = glm::inverse(camera_proj_matrix * camera_view_matrix);

    const auto near_ndc = glm::vec4{x_ndc, y_ndc, -1.0f, 1.0f};
    const auto far_ndc = glm::vec4{x_ndc, y_ndc, 1.0f, 1.0f};

    auto near_w = invPV * near_ndc;
    auto far_w = invPV * far_ndc;
    near_w /= near_w.w;
    far_w /= far_w.w;

    return Ray{
        .origin = glm::vec3(near_w),
        .dir = glm::normalize(glm::vec3(far_w - near_w)),
    };
}

/// https://pbr-book.org/4ed/Shapes/Spheres
[[nodiscard]] std::optional<f32>
intersect_ray_sphere(const Ray& ray, const ModelMatrix& model) noexcept
{
    const auto invM = glm::inverse(model);

    const Position3 origin_local{glm::vec3(invM * glm::vec4(ray.origin, 1.0f))};
    Direction3 dir_local{glm::vec3(invM * glm::vec4(ray.dir, 0.0f))};
    dir_local = glm::normalize(dir_local);

    const auto a = glm::dot(dir_local, dir_local);
    const auto b = 2.0f * glm::dot(origin_local, dir_local);
    const auto c = glm::dot(origin_local, origin_local) - 1.0f;

    const auto disc = b * b - 4.0f * a * c;
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

    const Position3 hit_local{origin_local + t_local * dir_local};
    const Position3 hit_world{glm::vec3(model * glm::vec4(hit_local, 1.0f))};

    const f32 t_world{glm::dot(hit_world - ray.origin, ray.dir)};
    if (t_world <= 0.0f)
    {
        return std::nullopt;
    }
    return t_world;
}

/// Solve ray.origin.z + t * ray.dir.z = 0 for t
[[nodiscard]] std::optional<f32> intersect_ray_ground(const Ray& ray)
{
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
intersect_ray_cube(const Ray& ray_world, const ModelMatrix& model) noexcept
{
    const auto invM = glm::inverse(model);
    const Position3 origin_local{glm::vec3(invM * glm::vec4(ray_world.origin, 1.0f))};
    const Direction3 dir_local{glm::vec3(invM * glm::vec4(ray_world.dir, 0.0f))};

    const Position3 bmin{-0.5f, -0.5f, -0.5f};
    const Position3 bmax{0.5f, 0.5f, 0.5f};

    f32 t_min{-1e30f};
    f32 t_max{1e30f};

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

    if (!slab(origin_local.x, dir_local.x, bmin.x, bmax.x))
    {
        return std::nullopt;
    }
    if (!slab(origin_local.y, dir_local.y, bmin.y, bmax.y))
    {
        return std::nullopt;
    }
    if (!slab(origin_local.z, dir_local.z, bmin.z, bmax.z))
    {
        return std::nullopt;
    }

    const auto tL = (t_min > 0.0f) ? t_min : ((t_max > 0.0f) ? t_max : -1.0f);
    if (tL <= 0.0f)
    {
        return std::nullopt;
    }

    const Position3 hit_local{origin_local + tL * dir_local};
    const Position3 hit_global{glm::vec3(model * glm::vec4(hit_local, 1.0f))};

    const f32 t_world{glm::dot(hit_global - ray_world.origin, ray_world.dir)};
    if (t_world <= 0.0f)
    {
        return std::nullopt;
    }

    return t_world;
}

}  // namespace ds_pba
