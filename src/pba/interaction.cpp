// pba/picking.cpp
#include "interaction.hpp"
#include "pba/types.hpp" // IWYU pragma: keep

#include <algorithm>
#include <cmath>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace ds_pba {

f32 max3(f32 a, f32 b, f32 c) {
    return std::max(a, std::max(b, c));
}

bool intersect_sphere(const Ray &ray, const glm::vec3 &center, f32 radius, f32 &t_hit) {
    const glm::vec3 oc = ray.origin - center;
    const f32 b = 2.0f * glm::dot(oc, ray.dir);
    const f32 c = glm::dot(oc, oc) - radius * radius;
    const f32 disc = b * b - 4.0f * c;
    if (disc < 0.0f) {
        return false;
    }

    const f32 s = std::sqrt(disc);
    const f32 t0 = (-b - s) * 0.5f;
    const f32 t1 = (-b + s) * 0.5f;

    const f32 t = (t0 > 0.0f) ? t0 : ((t1 > 0.0f) ? t1 : -1.0f);
    if (t <= 0.0f) {
        return false;
    }

    t_hit = t;
    return true;
}

Ray ray_from_mouse(
    GLFWwindow *window,
    f64 mouse_x,
    f64 mouse_y,
    const glm::mat4 &camera_view_matrix,
    const glm::mat4 &camera_proj_matrix) {
    // Use Framebuffer coords for NDC, makes a difference for retina displays
    int window_width = 1, window_height = 1;
    int framebuffer_width = 1, framebuffer_height = 1;
    glfwGetWindowSize(window, &window_width, &window_height);
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

    float window_width_f = static_cast<f32>(window_width);
    float window_height_f = static_cast<f32>(window_height);
    float framebuffer_width_f = static_cast<f32>(framebuffer_width);
    float framebuffer_height_f = static_cast<f32>(framebuffer_height);

    const f32 sx = (window_width_f > 0) ? (framebuffer_width_f / window_width_f) : 1.0f;
    const f32 sy = (window_height_f > 0) ? (framebuffer_height_f / window_height_f) : 1.0f;

    const f32 mx = static_cast<f32>(mouse_x) * sx;
    const f32 my = static_cast<f32>(mouse_y) * sy;

    const f32 x_ndc = (framebuffer_width_f > 0) ? (2.0f * mx / framebuffer_width_f - 1.0f) : 0.0f;
    const f32 y_ndc = (framebuffer_height_f > 0) ? (1.0f - 2.0f * my / framebuffer_height_f) : 0.0f;

    const glm::mat4 invPV = glm::inverse(camera_proj_matrix * camera_view_matrix);

    glm::vec4 near_ndc(x_ndc, y_ndc, -1.0f, 1.0f);
    glm::vec4 far_ndc(x_ndc, y_ndc, 1.0f, 1.0f);

    glm::vec4 near_w = invPV * near_ndc;
    glm::vec4 far_w = invPV * far_ndc;
    near_w /= near_w.w;
    far_w /= far_w.w;

    return Ray{
        .origin = glm::vec3(near_w),
        .dir = glm::normalize(glm::vec3(far_w - near_w)),
    };
}

Ray ray_from_mouse_in_rect(
    GLFWwindow *window,
    f64 mouse_x,
    f64 mouse_y,
    int rect_x,
    int rect_y,
    int rect_w,
    int rect_h,
    const glm::mat4 &camera_view_matrix,
    const glm::mat4 &camera_proj_matrix) {

    int fbw = 1, fbh = 1;
    glfwGetFramebufferSize(window, &fbw, &fbh);

    int win_w = 1, win_h = 1;
    glfwGetWindowSize(window, &win_w, &win_h);

    const f32 sx = (win_w > 0) ? (static_cast<f32>(fbw) / static_cast<f32>(win_w)) : 1.0f;
    const f32 sy = (win_h > 0) ? (static_cast<f32>(fbh) / static_cast<f32>(win_h)) : 1.0f;

    const f32 mx_fb = static_cast<f32>(mouse_x) * sx;
    const f32 my_fb = static_cast<f32>(mouse_y) * sy;

    const f32 lx = mx_fb - static_cast<f32>(rect_x);
    const f32 ly = my_fb - static_cast<f32>(fbh - (rect_y + rect_h));

    const f32 x_ndc = (rect_w > 0) ? (2.0f * lx / static_cast<f32>(rect_w) - 1.0f) : 0.0f;
    const f32 y_ndc = (rect_h > 0) ? (1.0f - 2.0f * ly / static_cast<f32>(rect_h)) : 0.0f;

    const glm::mat4 invPV = glm::inverse(camera_proj_matrix * camera_view_matrix);

    glm::vec4 near_ndc(x_ndc, y_ndc, -1.0f, 1.0f);
    glm::vec4 far_ndc(x_ndc, y_ndc, 1.0f, 1.0f);

    glm::vec4 near_w = invPV * near_ndc;
    glm::vec4 far_w = invPV * far_ndc;
    near_w /= near_w.w;
    far_w /= far_w.w;

    return Ray{
        .origin = glm::vec3(near_w),
        .dir = glm::normalize(glm::vec3(far_w - near_w)),
    };
}

Ray ray_from_imgui_rect(
    const glm::vec2 &mouse_pos,
    const glm::vec2 &rect_pos,
    const glm::vec2 &rect_size,
    const glm::mat4 &camera_view_matrix,
    const glm::mat4 &camera_proj_matrix) {

    const float lx = mouse_pos.x - rect_pos.x;
    const float ly = mouse_pos.y - rect_pos.y;

    const float w = std::max(1.0f, rect_size.x);
    const float h = std::max(1.0f, rect_size.y);

    const float x_ndc = 2.0f * (lx / w) - 1.0f;
    const float y_ndc = 1.0f - 2.0f * (ly / h);

    const glm::mat4 invPV = glm::inverse(camera_proj_matrix * camera_view_matrix);

    glm::vec4 near_ndc(x_ndc, y_ndc, -1.0f, 1.0f);
    glm::vec4 far_ndc(x_ndc, y_ndc, 1.0f, 1.0f);

    glm::vec4 near_w = invPV * near_ndc;
    glm::vec4 far_w = invPV * far_ndc;
    near_w /= near_w.w;
    far_w /= far_w.w;

    return ds_pba::Ray{
        .origin = glm::vec3(near_w),
        .dir = glm::normalize(glm::vec3(far_w - near_w)),
    };
}

bool intersect_unit_cube_obb(
    const Ray &ray_world,
    const glm::mat4 &model,
    f32 &t_world_out) {
    const glm::mat4 invM = glm::inverse(model);

    const glm::vec3 oL = glm::vec3(invM * glm::vec4(ray_world.origin, 1.0f));
    const glm::vec3 dL = glm::vec3(invM * glm::vec4(ray_world.dir, 0.0f));

    const glm::vec3 bmin(-0.5f), bmax(0.5f);

    f32 tmin = -1e30f;
    f32 tmax = 1e30f;

    auto slab = [&](f32 o, f32 d, f32 mn, f32 mx) -> bool {
        if (std::abs(d) < 1e-8f) {
            return (o >= mn && o <= mx);
        }
        f32 t1 = (mn - o) / d;
        f32 t2 = (mx - o) / d;
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        return tmin <= tmax;
    };

    if (!slab(oL.x, dL.x, bmin.x, bmax.x)) {
        return false;
    }
    if (!slab(oL.y, dL.y, bmin.y, bmax.y)) {
        return false;
    }
    if (!slab(oL.z, dL.z, bmin.z, bmax.z)) {
        return false;
    }

    f32 tL = (tmin > 0.0f) ? tmin : ((tmax > 0.0f) ? tmax : -1.0f);
    if (tL <= 0.0f) {
        return false;
    }

    const glm::vec3 hitL = oL + tL * dL;
    const glm::vec3 hitW = glm::vec3(model * glm::vec4(hitL, 1.0f));

    const f32 tW = glm::dot(hitW - ray_world.origin, ray_world.dir);
    if (tW <= 0.0f) {
        return false;
    }

    t_world_out = tW;
    return true;
}

} // namespace ds_pba