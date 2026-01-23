// pba/scene_types.hpp
#pragma once

#include "glm/ext/matrix_transform.hpp"
#include "pba/core_types.hpp"  // IWYU pragma: keep
#include "pba/math_types.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace ds_pba
{

struct Transform
{
    Position3 position{0.0f, 0.0f, 0.0f};
    EulerDeg3 rotation_deg{0.0f, 0.0f, 0.0f};
    Direction3 scale{1.0f, 1.0f, 1.0f};

    [[nodiscard]] ModelMatrix model_matrix() const
    {
        auto M{glm::identity<ModelMatrix>()};
        M = glm::translate(M, position);
        M = glm::rotate(M, glm::radians(rotation_deg.z), glm::vec3(0, 0, 1));
        M = glm::rotate(M, glm::radians(rotation_deg.y), glm::vec3(0, 1, 0));
        M = glm::rotate(M, glm::radians(rotation_deg.x), glm::vec3(1, 0, 0));
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
    ObjectId id;
    ObjectType type;
    Transform transform{};
    glm::vec3 color{0.8f, 0.8f, 0.8f};
};

}  // namespace ds_pba
