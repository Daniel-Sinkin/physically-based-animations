// pba/raycast.cpp
#include "pba/math_types.hpp"
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/raycast.hpp"
//
#include "pba/scene_context.hpp"

namespace ds_pba
{
std::optional<Raycast> raycast(const SceneContext& scene_context, const Ray& ray)
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

    if (!best_idx)
    {
        return std::nullopt;
    }
    assert(best_t);
    assert(best_type);

    return Raycast{
        .ray = ray,
        .hit = ray.origin + best_t * ray.dir,
        .t = best_t,
        .object_id = *best_object_id,
        .object_type = *best_type
    };
}

Ray ray_from_mouse(
    GLFWwindow* window,
    f64 mouse_x,
    f64 mouse_y,
    const ViewMatrix& camera_view_matrix,
    const ProjMatrix& camera_proj_matrix
)
{
    int window_width{0};
    int window_height{0};
    int framebuffer_width{0};
    int framebuffer_height{0};
    glfwGetWindowSize(window, &window_width, &window_height);
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    const auto window_width_f = static_cast<f32>(window_width);
    const auto window_height_f = static_cast<f32>(window_height);
    const auto fb_width_f = static_cast<f32>(framebuffer_width);
    const auto fb_height_f = static_cast<f32>(framebuffer_height);

    const f32 scale_x{(window_width_f > 0) ? (fb_width_f / window_width_f) : 1.0f};
    const f32 scale_y{(window_height_f > 0) ? (fb_height_f / window_height_f) : 1.0f};

    const auto mx = static_cast<f32>(mouse_x) * scale_x;
    const auto my = static_cast<f32>(mouse_y) * scale_y;

    const f32 x_ndc{(fb_width_f > 0) ? (2.0f * mx / fb_width_f - 1.0f) : 0.0f};
    const f32 y_ndc{(fb_height_f > 0) ? (1.0f - 2.0f * my / fb_height_f) : 0.0f};

    const glm::mat4 invPV{glm::inverse(camera_proj_matrix * camera_view_matrix)};

    const glm::vec4 near_ndc{x_ndc, y_ndc, -1.0f, 1.0f};
    const glm::vec4 far_ndc{x_ndc, y_ndc, 1.0f, 1.0f};

    glm::vec4 near_w{invPV * near_ndc};
    glm::vec4 far_w{invPV * far_ndc};
    near_w /= near_w.w;
    far_w /= far_w.w;

    return Ray{
        .origin = glm::vec3(near_w),
        .dir = glm::normalize(glm::vec3(far_w - near_w)),
    };
}

Ray ray_from_imgui_rect(
    const glm::vec2& mouse_pos,
    const glm::vec2& rect_pos,
    const glm::vec2& rect_size,
    const ViewMatrix& camera_view_matrix,
    const ProjMatrix& camera_proj_matrix
)
{
    const f32 lx{mouse_pos.x - rect_pos.x};
    const f32 ly{mouse_pos.y - rect_pos.y};

    const f32 w{std::max(1.0f, rect_size.x)};
    const f32 h{std::max(1.0f, rect_size.y)};

    const f32 x_ndc{2.0f * (lx / w) - 1.0f};
    const f32 y_ndc{1.0f - 2.0f * (ly / h)};

    const glm::mat4 invPV{glm::inverse(camera_proj_matrix * camera_view_matrix)};

    const glm::vec4 near_ndc(x_ndc, y_ndc, -1.0f, 1.0f);
    const glm::vec4 far_ndc(x_ndc, y_ndc, 1.0f, 1.0f);

    glm::vec4 near_w = invPV * near_ndc;
    glm::vec4 far_w = invPV * far_ndc;
    near_w /= near_w.w;
    far_w /= far_w.w;

    return Ray{
        .origin = glm::vec3(near_w),
        .dir = glm::normalize(glm::vec3(far_w - near_w)),
    };
}

/// https://pbr-book.org/4ed/Shapes/Spheres
std::optional<f32> intersect_ray_sphere(const Ray& ray, const ModelMatrix& model)
{
    const glm::mat4 invM = glm::inverse(model);

    const Position3 origin_local{glm::vec3(invM * glm::vec4(ray.origin, 1.0f))};
    Direction3 dir_local{glm::vec3(invM * glm::vec4(ray.dir, 0.0f))};
    dir_local = glm::normalize(dir_local);

    const f32 a{glm::dot(dir_local, dir_local)};
    const f32 b{2.0f * glm::dot(origin_local, dir_local)};
    const f32 c{glm::dot(origin_local, origin_local) - 1.0f};

    const f32 disc{b * b - 4.0f * a * c};
    if (disc < 0.0f)
    {
        return std::nullopt;
    }

    const f32 s{std::sqrt(disc)};
    const f32 inv2a{0.5f / a};
    const f32 t0{(-b - s) * inv2a};
    const f32 t1{(-b + s) * inv2a};

    const f32 t_local{(t0 > 0.0f) ? t0 : ((t1 > 0.0f) ? t1 : -1.0f)};
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
std::optional<f32> intersect_ray_ground(const Ray& ray)
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
std::optional<f32> intersect_ray_cube(const Ray& ray_world, const ModelMatrix& model)
{
    const glm::mat4 invM{glm::inverse(model)};
    const Position3 origin_local{glm::vec3(invM * glm::vec4(ray_world.origin, 1.0f))};
    const Direction3 dir_local{glm::vec3(invM * glm::vec4(ray_world.dir, 0.0f))};

    const Position3 bmin{-0.5f};
    const Position3 bmax{0.5f};

    f32 tmin{-1e30f};
    f32 tmax{1e30f};

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
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        return tmin <= tmax;
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

    const f32 tL = (tmin > 0.0f) ? tmin : ((tmax > 0.0f) ? tmax : -1.0f);
    if (tL <= 0.0f)
    {
        return std::nullopt;
    }

    const Position3 hit_local{origin_local + tL * dir_local};
    const Position3 hit_global{glm::vec3(model * glm::vec4(hit_local, 1.0f))};

    const f32 tW{glm::dot(hit_global - ray_world.origin, ray_world.dir)};
    if (tW <= 0.0f)
    {
        return std::nullopt;
    }

    return tW;
}

}  // namespace ds_pba
