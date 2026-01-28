// pba/gfx/gfx_object_handle.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/gfx/gfx_context.hpp"
//
#include "pba/core/core_types.hpp"
#include "pba/core/format.hpp"  // IWYU pragma: keep
#include "pba/core/math_types.hpp"
#include "pba/engine/engine_context.hpp"
#include "pba/gfx/raycast.hpp"
#include "pba/scene/scene_context.hpp"

#include "pba/ui/ui.hpp"
//
#include <imgui.h>
#include <optional>
#include <utility>
//
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <json.hpp>

namespace ds_pba
{
std::optional<Pos3> GfxContext::get_object_position(EntityId id) const
{
    if (!scene_context)
    {
        return std::nullopt;
    }

    if (engine_context)
    {
        if (auto it = engine_context->obj_map.find(id); it != engine_context->obj_map.end())
        {
            const auto [scene_i, phys_i] = it->second;
            if (phys_i < engine_context->physics.bodies.size())
            {
                return engine_context->physics.bodies[phys_i].position;
            }
            if (scene_i < scene_context->cube_objects.size())
            {
                return scene_context->cube_objects[scene_i].transform.position;
            }
        }
    }

    for (const auto& o : scene_context->cube_objects)
    {
        if (o.id == id)
        {
            return o.transform.position;
        }
    }
    for (const auto& o : scene_context->sphere_objects)
    {
        if (o.id == id)
        {
            return o.transform.position;
        }
    }
    for (const auto& o : scene_context->hitmarker_objects)
    {
        if (o.id == id)
        {
            return o.transform.position;
        }
    }

    return std::nullopt;
}
void GfxContext::set_object_position(EntityId id, const Pos3& p) const
{
    if (!scene_context)
    {
        return;
    }

    // Physics-backed cubes
    if (engine_context)
    {
        if (auto it = engine_context->obj_map.find(id); it != engine_context->obj_map.end())
        {
            const auto [scene_i, phys_i] = it->second;

            if (phys_i < engine_context->physics.bodies.size())
            {
                RigidBody& rb = engine_context->physics.bodies[phys_i];
                rb.position = p;

                rb.velocity = Dir3{};
                rb.angular_velocity = Dir3{};
                rb.force_accum = Dir3{};
                rb.torque_accum = Dir3{};

                rb.asleep = false;
                rb.sleep_frames = 0;
            }

            if (scene_i < scene_context->cube_objects.size())
            {
                scene_context->cube_objects[scene_i].transform.position = p;
            }
            return;
        }
    }

    // Non-physics objects (spheres/hitmarkers)
    for (auto& o : scene_context->sphere_objects)
    {
        if (o.id == id)
        {
            o.transform.position = p;
            return;
        }
    }
    for (auto& o : scene_context->hitmarker_objects)
    {
        if (o.id == id)
        {
            o.transform.position = p;
            return;
        }
    }
}
}  // namespace ds_pba
