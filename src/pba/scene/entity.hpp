// pba/scene/entity.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/scene/world_types.hpp"

#include <optional>
#include <string>
#include <utility>

namespace ds_pba
{
struct BodyHandle
{
    u32 index{};
    u32 generation{};
};

enum class EntityType
{
    Cube,
};

[[nodiscard]] constexpr std::string_view to_string(EntityType type) noexcept
{
    switch (type)
    {
        case EntityType::Cube:
            return "Cube";
    }
    std::unreachable();
}

using EntityId = u32;
inline constexpr EntityId k_invalid_id{std::numeric_limits<EntityId>::max()};

struct Entity
{
    EntityId id{k_invalid_id};
    EntityType type{EntityType::Cube};

    Transform transform{};
    Color3 color{k_scene_object_default_color};

    std::optional<BodyHandle> body{};
    std::string name{};
};
}  // namespace ds_pba
