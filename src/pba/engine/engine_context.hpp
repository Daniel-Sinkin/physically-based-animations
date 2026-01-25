// pba/engine/engine_context.hpp
#pragma once

#include "pba/core/core_types.hpp"
#include "pba/core/math_types.hpp"
#include "pba/engine/scene_context.hpp"
#include "pba/physics/physics_context.hpp"
#include "pba/gfx/render_context.hpp"

#include <string>
#include <unordered_map>

namespace ds_pba
{

struct EngineContext
{
    struct ObjectLink
    {
        usize cube_obj_idx;
        usize physics_obj_idx;
    };

    SceneContext scene{};
    Renderer renderer{};
    PhysicsContext physics{};

    std::unordered_map<ObjectId, ObjectLink> obj_map{};
    std::unordered_map<ObjectId, std::string> obj_name_map{};

    TimePoint frame_time = Clock::now();
    Duration accumulator{};

    bool paused{true};
    bool prev_space{false};

    void link_latest_objects(ObjectId id);

    void add_cube(Position3 position);
    void add_ground();

    bool setup();
    void run();
};

}  // namespace ds_pba
