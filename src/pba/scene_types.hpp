// pba/scene_types.hpp
#pragma once

#include "pba/core_types.hpp"  // IWYU pragma: keep

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace ds_pba
{

struct Transform
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation_deg{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    [[nodiscard]] glm::mat4 model_matrix() const
    {
        glm::mat4 M(1.0f);
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
    Sphere
};
struct Object
{
    ObjectId id;
    ObjectType type;
    Transform transform{};
    glm::vec3 color{0.8f, 0.8f, 0.8f};
};

}  // namespace ds_pba
