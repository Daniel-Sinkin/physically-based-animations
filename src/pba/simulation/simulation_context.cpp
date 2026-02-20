// pba/simulation/simulation_context.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/simulation/simulation_context.hpp"
//

namespace ds_pba
{
namespace
{

[[nodiscard]] auto inv_inertia_body_box(f32 inv_mass, const Dir3& half_extents) noexcept
    -> glm::mat3
{
    if (inv_mass <= k_static_mass)
    {
        return glm::mat3(0.0f);
    }

    Expects(inv_mass > 0.0f);
    if (inv_mass <= 0.0f)
    {
        return glm::mat3(0.0f);
    }
    const auto m = 1.0f / inv_mass;

    const auto x = 2.0f * half_extents.x;
    const auto y = 2.0f * half_extents.y;
    const auto z = 2.0f * half_extents.z;

    const auto Ixx = (m / 12.0f) * (y * y + z * z);
    const auto Iyy = (m / 12.0f) * (x * x + z * z);
    const auto Izz = (m / 12.0f) * (x * x + y * y);

    glm::mat3 invI{0.0f};
    invI[0][0] = (Ixx > 0.0f) ? (1.0f / Ixx) : 0.0f;
    invI[1][1] = (Iyy > 0.0f) ? (1.0f / Iyy) : 0.0f;
    invI[2][2] = (Izz > 0.0f) ? (1.0f / Izz) : 0.0f;
    return invI;
}

[[nodiscard]] auto
inv_inertia_world_from_body(const Quaternion& q, const glm::mat3& inv_inertia_body) noexcept
    -> glm::mat3
{
    const glm::mat3 R{glm::mat3_cast(q)};
    return R * inv_inertia_body * glm::transpose(R);
}

auto init_box_inertia(RigidBody& b) noexcept -> void
{
    b.inv_inertia_body = inv_inertia_body_box(b.inv_mass, b.half_extents);
    b.inv_inertia_world = inv_inertia_world_from_body(b.orientation, b.inv_inertia_body);
}

}  // namespace

auto SimulationContext::spawn_cube(
    Pos3 pos,
    Dir3 half_extents,
    f32 inv_mass,
    Dir3 vel,
    Quaternion ori,
    Dir3 ang_vel,
    Color3 color,
    std::string_view name
) -> Entity&
{
    Entity& e = world.spawn(
        EntityType::Cube,
        Transform{.position = pos, .scale = half_extents * 2.0f, .orientation = ori},
        color
    );

    if (!name.empty())
    {
        e.name = std::string{name};
    }

    RigidBody rb{
        .id = e.id,
        .half_extents = half_extents,
        .position = pos,
        .velocity = vel,
        .inv_mass = inv_mass,
        .orientation = ori,
        .angular_velocity = ang_vel,
    };
    init_box_inertia(rb);

    e.body = physics.add_body(std::move(rb));
    return e;
}

auto SimulationContext::add_ground() -> Entity&
{
    return spawn_cube(
        Pos3{0.0f, 0.0f, -3.5f},
        Dir3{10.0f, 10.0f, 0.5f},
        k_static_mass,
        k_zero_dir,
        k_quaternion_identity,
        k_zero_dir,
        Color3{0.1f, 0.1f, 0.1f},
        "Ground"
    );
}

auto SimulationContext::create_pyramid(int base_n, f32 step_size, f32 base_z) -> void
{
    for (int layer{0}; layer < base_n; ++layer)
    {
        const auto n = base_n - layer;
        const auto z = base_z + static_cast<f32>(layer) * step_size;
        const auto half_span = 0.5f * static_cast<f32>(n - 1) * step_size;

        for (int i{0}; i < n; ++i)
        {
            const auto x = static_cast<f32>(i) * step_size - half_span;

            auto& entity = spawn_cube(
                Pos3{x, 0.0f, z},
                Dir3{0.5f, 0.5f, 0.5f},
                1.0f,
                k_zero_dir,
                k_quaternion_identity,
                k_zero_dir,
                Color3{k_scene_object_default_color},
                {}
            );

            entity.name = std::format("Pyramid (layer={}, idx={})", layer, i);
        }
    }
}
auto SimulationContext::create_pyramid_3d(int base_n, f32 step_size, f32 base_z) -> void
{
    for (int layer{0}; layer < base_n; ++layer)
    {
        const auto n = base_n - layer;
        const auto z = base_z + static_cast<f32>(layer) * step_size;
        const auto half_span = 0.5f * static_cast<f32>(n - 1) * step_size;

        for (int i{0}; i < n; ++i)
        {
            const auto x = static_cast<f32>(i) * step_size - half_span;

            for (int j{0}; j < n; ++j)
            {
                const auto y = static_cast<f32>(j) * step_size - half_span;

                auto& e = spawn_cube(
                    Pos3{x, y, z},
                    Dir3{0.5f, 0.5f, 0.5f},
                    1.0f,
                    k_zero_dir,
                    k_quaternion_identity,
                    k_zero_dir,
                    Color3{k_scene_object_default_color},
                    {}
                );

                e.name = std::format("Pyramid3D (layer={}, ix={}, iy={})", layer, i, j);
            }
        }
    }
}

auto SimulationContext::sync_physics_to_world() -> void
{
    for (auto i = 0zu; i < world.entities().size(); ++i)
    {
        auto& e = world.entity(i);
        if (!e.body)
        {
            continue;
        }
        if (const auto rb = physics.try_body(*e.body))
        {
            auto t = e.transform;
            t.position = rb->position;
            t.orientation = rb->orientation;
            (void) world.set_transform_at(i, t);
        }
    }
}
auto SimulationContext::create_box_body(const Entity& e, f32 inv_mass, Dir3 velo) const -> RigidBody
{

    RigidBody rb{
        .id = e.id,
        .half_extents = e.transform.scale * 0.5f,
        .position = e.transform.position,
        .velocity = velo,
        .inv_mass = inv_mass,
        .orientation = e.transform.orientation
    };
    init_box_inertia(rb);
    return rb;
}
}  // namespace ds_pba
