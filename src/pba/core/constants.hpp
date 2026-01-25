// pba/core/constants.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"

#include <numbers>

namespace ds_pba
{
//
// Memory
//
inline constexpr usize k_kib{1024zu};
inline constexpr usize k_mib{1024zu * k_kib};

//
// Development
//
constexpr bool k_validate_contacts{false};

//
// Video Recorder
//
constexpr int k_video_recorder_fps{60};

//
// Input Handling
//
constexpr f32 k_zoom_speed{0.12f};
constexpr f32 k_sensitivity{0.0050f};
constexpr f32 k_pan_sensitivity{1.00f};

//
// Common
//
inline constexpr f32 k_pi{std::numbers::pi_v<f32>};
inline constexpr f32 k_two_pi{2.0f * std::numbers::pi_v<f32>};

inline constexpr Direction3 k_axis_x{1.0f, 0.0f, 0.0f};
inline constexpr Direction3 k_axis_y{0.0f, 1.0f, 0.0f};
inline constexpr Direction3 k_axis_z{0.0f, 0.0f, 1.0f};

//
// Ui
//
inline constexpr usize k_max_terminal_lines{2000};

//
// Physics
//
inline constexpr Direction3 k_gravity{0.0f, 0.0f, -9.81f};
inline constexpr usize k_contact_points{8};
inline constexpr usize k_collision_reduced_num{4zu};

constexpr f32 k_linear_damping{0.2f};
constexpr f32 k_angular_damping{1.5f};
constexpr f32 k_linear_sleep_speed_threshold{0.25f};
constexpr f32 k_angular_sleep_speed_threshold{1.0f};

constexpr int k_solver_iterations{20};
constexpr int k_position_iterations{4};
constexpr f32 k_restitution{0.1f};

constexpr f32 k_pen_percent{0.05f};
constexpr f32 k_pen_max_correction{0.05f};
// This corresponds to [Box2D] linearSlop parameter
constexpr f32 k_pen_tolerance{0.005f};
constexpr f32 k_pen_correction_frag{0.1f};

constexpr f32 k_friction{0.5f};

inline constexpr usize k_physics_step_arena_bytes{1zu * k_mib};

//
// Camera
//
inline constexpr Position3 k_camera_pivot{0.0f, 0.0f, 0.0f};
inline constexpr f32 k_camera_distance{25.0f};
inline constexpr f32 k_camera_yaw{50.0f};
inline constexpr f32 k_camera_pitch{30.0f};
inline constexpr f32 k_camera_fov_y{60.0f};
inline constexpr f32 k_camera_z_near{0.1f};
inline constexpr f32 k_camera_z_far{1000.0f};

//
// Render
//
inline constexpr ColorRGBf k_scene_object_default_color{0.8f, 0.8f, 0.8f};
inline constexpr int k_num_lines_per_side{40};
inline constexpr f32 k_spacing{1.0f};
inline constexpr f32 k_fog_start{25.0f};
inline constexpr f32 k_fog_end{40.0f};
inline constexpr f32 k_minor_alpha{0.35f};
inline constexpr f32 k_axis_alpha{0.95f};

//
//
//
//
//
//

namespace detail
{  // Guardrails and invariants
static_assert(k_video_recorder_fps > 0);

static_assert(k_zoom_speed > 0.0f);
static_assert(k_sensitivity > 0.0f);
static_assert(k_pan_sensitivity > 0.0f);

static_assert(k_contact_points > 0zu);
static_assert(k_collision_reduced_num > 0zu);

static_assert(k_linear_damping >= 0.0f);
static_assert(k_angular_damping >= 0.0f);
static_assert(k_linear_sleep_speed_threshold >= 0.0f);
static_assert(k_angular_sleep_speed_threshold >= 0.0f);

static_assert(k_solver_iterations > 0);
static_assert(k_position_iterations > 0);

static_assert(k_restitution >= 0.0f && k_restitution <= 1.0f);

static_assert(k_pen_percent > 0.0f && k_pen_percent <= 1.0f);
static_assert(k_pen_max_correction > 0.0f);
static_assert(k_pen_tolerance >= 0.0f);
static_assert(k_pen_correction_frag >= 0.0f && k_pen_correction_frag <= 1.0f);

static_assert(k_friction >= 0.0f);

static_assert(k_camera_distance > 0.0f);
static_assert(k_camera_fov_y > 0.0f && k_camera_fov_y < 180.0f);
static_assert(k_camera_z_near > 0.0f);
static_assert(k_camera_z_far > k_camera_z_near);

static_assert(k_fog_end > k_fog_start);
static_assert(k_minor_alpha >= 0.0f && k_minor_alpha <= 1.0f);
static_assert(k_axis_alpha >= 0.0f && k_axis_alpha <= 1.0f);
}  // namespace detail
}  // namespace ds_pba
