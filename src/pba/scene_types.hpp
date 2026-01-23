// pba/scene_types.hpp
#pragma once

#include "pba/core_types.hpp"  // IWYU pragma: keep
#include "pba/math_types.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ds_pba
{

struct Transform
{
    Position3 position{0.0f, 0.0f, 0.0f};
    Direction3 scale{1.0f, 1.0f, 1.0f};
    Quaternion orientation{1.0f, 0.0f, 0.0f, 0.0f};

    [[nodiscard]] ModelMatrix model_matrix() const
    {
        auto M{glm::identity<ModelMatrix>()};
        M = glm::translate(M, position);
        M *= glm::mat4_cast(orientation);
        M = glm::scale(M, scale);
        return M;
    }
};

enum class ObjectType
{
    Cube,
    Sphere,
    Hitmarker
};
struct Object
{  // Prolly should be a render type
    ObjectId id{k_invalid_id};
    ObjectType type{ObjectType::Cube};
    Transform transform{};
    glm::vec3 color{0.8f, 0.8f, 0.8f};
};

}  // namespace ds_pba
