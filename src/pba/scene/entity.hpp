// pba/scene/entity.hpp
#pragma once

#include "pba/core/arena_allocator.hpp"
#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/physics/physics_types.hpp"
#include "pba/scene/entity_id.hpp"
#include "pba/scene/world_types.hpp"

#include <string>
#include <utility>

namespace ds_pba
{
enum class EntityType
{
    Cube,
};

[[nodiscard]] constexpr auto to_string(EntityType type) noexcept -> std::string_view
{
    switch (type)
    {
        case EntityType::Cube:
            return "Cube";
    }
    std::unreachable();
}

struct Entity
{
    EntityId id{k_invalid_id};
    EntityType type{EntityType::Cube};

    Transform transform{};
    Color3 color{k_scene_object_default_color};

    std::optional<BodyHandle> body{};
    std::string name{};
};

struct EntityAOS
{
    EntityId id{k_invalid_id};
    EntityType type{EntityType::Cube};
    std::optional<BodyHandle> body{};
    std::string name{};
};
}  // namespace ds_pba
