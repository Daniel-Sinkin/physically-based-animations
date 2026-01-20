// serialisation.hpp
#pragma once

#include "camera.hpp"
#include "pba/types.hpp"  // IWYU pragma: keep
#include "scene_context.hpp"

#include <nlohmann/json.hpp>

namespace nlohmann
{
/// Hack to work around ADL limitations when serializing third-party types, basically
/// the way that namespaces are resolves means that formatter will always look inside of
/// (for example) glm instead of in ours as the type is defined there.
/// https://json.nlohmann.me/api/adl_serializer/
template <>
struct adl_serializer<glm::vec3>
{
    static void to_json(json& j, const glm::vec3& v)
    {
        j = json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
    }

    static void from_json(const json& j, glm::vec3& v)
    {
        j.at("x").get_to(v.x);
        j.at("y").get_to(v.y);
        j.at("z").get_to(v.z);
    }
};

}  // namespace nlohmann

namespace ds_pba
{

void to_json(nlohmann::json& j, const glm::vec3& v);
void from_json(const nlohmann::json& j, glm::vec3& v);

void to_json(nlohmann::json& j, const Transform& t);
void from_json(const nlohmann::json& j, Transform& t);

void to_json(nlohmann::json& j, const Object& o);
void from_json(const nlohmann::json& j, Object& o);

void to_json(nlohmann::json& j, const Camera& c);
void from_json(const nlohmann::json& j, Camera& c);

void to_json(nlohmann::json& j, const SceneContext& s);
void from_json(const nlohmann::json& j, SceneContext& s);

}  // namespace ds_pba
