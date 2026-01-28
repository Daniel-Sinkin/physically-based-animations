// pba/scene/scene_types.hpp
#pragma once

#include "pba/core/constants.hpp"
#include "pba/core/core_types.hpp"  // IWYU pragma: keep
#include "pba/core/geometry.hpp"
#include "pba/core/math_types.hpp"
//
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ds_pba
{

struct Transform
{
    Pos3 position{};
    Dir3 scale{1.0f, 1.0f, 1.0f};
    Quaternion orientation{k_quaternion_identity};

    [[nodiscard]] ModelMatrix model_matrix() const noexcept
    {
        auto M{glm::identity<glm::mat4>()};
        M = glm::translate(M, position);
        M *= glm::mat4_cast(orientation);
        M = glm::scale(M, scale);
        return ModelMatrix{M};
    }
};

enum class ObjectType
{
    Cube,
    Sphere,
    Hitmarker,
    MarbleBust
};
struct Object
{
    ObjectId id{k_invalid_id};
    ObjectType type{ObjectType::Cube};
    Transform transform{};
    Color3 color{k_scene_object_default_color};
};

}  // namespace ds_pba
