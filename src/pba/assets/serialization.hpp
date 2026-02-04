// pba/assets/serialization.hpp
#pragma once

#include "pba/scene/world_types.hpp"

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <json.hpp>

namespace nlohmann
{
template <>
struct adl_serializer<glm::vec2>
{
    static auto to_json(json& j, const glm::vec2& v) -> void
    {
        j = json::array({v.x, v.y});
    }

    static auto from_json(const json& j, glm::vec2& v) -> void
    {
        if (!j.is_array() || j.size() != 2)
        {
            throw json::type_error::create(302, "glm::vec2 must be array[2]", j);
        }
        v.x = j.at(0).get<float>();
        v.y = j.at(1).get<float>();
    }
};

template <>
struct adl_serializer<glm::vec3>
{
    static auto to_json(json& j, const glm::vec3& v) -> void
    {
        j = json::array({v.x, v.y, v.z});
    }

    static auto from_json(const json& j, glm::vec3& v) -> void
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

template <>
struct adl_serializer<glm::quat>
{
    static auto to_json(json& j, const glm::quat& q) -> void
    {
        j = json::array({q.w, q.x, q.y, q.z});
    }

    static auto from_json(const json& j, glm::quat& q) -> void
    {
        if (!j.is_array() || j.size() != 4)
        {
            throw json::type_error::create(302, "glm::quat must be array[4] (w,x,y,z)", j);
        }
        q.w = j.at(0).get<float>();
        q.x = j.at(1).get<float>();
        q.y = j.at(2).get<float>();
        q.z = j.at(3).get<float>();
    }
};

}  // namespace nlohmann

namespace ds_pba
{

inline auto to_json(nlohmann::json& j, const Transform& t) -> void
{
    j = nlohmann::json::object();
    j["position"] = t.position;
    j["scale"] = t.scale;
    j["orientation"] = t.orientation;
}

inline auto from_json(const nlohmann::json& j, Transform& t) -> void
{
    if (!j.is_object())
    {
        throw nlohmann::json::type_error::create(302, "Transform must be object", j);
    }

    t.position = j.at("position").get<glm::vec3>();
    t.scale = j.at("scale").get<glm::vec3>();
    t.orientation = j.at("orientation").get<glm::quat>();
}

}  // namespace ds_pba
