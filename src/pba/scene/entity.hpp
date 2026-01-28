// pba/scene/entity.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"
#include "pba/scene/world_types.hpp"

#include <optional>
#include <string>

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
    Sphere,
    Hitmarker,
    MarbleBust
};

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

[[nodiscard]] inline EntityId next_object_id() noexcept
{
    static std::atomic<EntityId> counter{0};
    const EntityId id{counter.fetch_add(1u, std::memory_order_relaxed)};
    if (id == k_invalid_id)
    {
        std::println(stderr, "Generated invalid id");
        std::abort();
    }
    return id;
};
}  // namespace ds_pba
