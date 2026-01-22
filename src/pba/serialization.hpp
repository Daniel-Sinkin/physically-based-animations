// pba/serialization.hpp
#pragma once

#include "pba/scene_types.hpp"

#include <json.hpp>

namespace nlohmann
{

template <>
struct adl_serializer<glm::vec3>
{
    static void to_json(json& j, const glm::vec3& v)
    {
        j = json::array({v.x, v.y, v.z});
    }

    static void from_json(const json& j, glm::vec3& v)
    {
        if (!j.is_array() || j.size() != 3)
        {
            throw json::type_error::create(302, "glm::vec3 must be array[3]", j);
        }
        v.x = j.at(0).get<float>();
        v.y = j.at(1).get<float>();
        v.z = j.at(2).get<float>();
    }
};

}  // namespace nlohmann

namespace ds_pba
{

inline void to_json(nlohmann::json& j, const Transform& t)
{
    j = nlohmann::json::object();
    j["position"] = t.position;
    j["rotation_deg"] = t.rotation_deg;
    j["scale"] = t.scale;
}

inline void from_json(const nlohmann::json& j, Transform& t)
{
    if (!j.is_object())
    {
        throw nlohmann::json::type_error::create(302, "Transform must be object", j);
    }

    t.position = j.at("position").get<glm::vec3>();
    t.rotation_deg = j.at("rotation_deg").get<glm::vec3>();
    t.scale = j.at("scale").get<glm::vec3>();
}

}  // namespace ds_pba
