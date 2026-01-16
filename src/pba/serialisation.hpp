// serialisation.hpp
#pragma once

#include <nlohmann/json.hpp>

#include "camera.hpp"
#include "scene_context.hpp"
#include "types.hpp"

namespace nlohmann {
// Hack to alleviate ADL issues, see https://en.cppreference.com/w/cpp/language/adl.html
// The problem is that the overloads MUST look in the library namespace (e.g. glm) instead of ours,
// so they miss our implementation
template <>
struct adl_serialiser<glm::vec3> {
    static void to_json(json& j, const glm::vec3& v) {
        j = json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
    }

    static void from_json(const json& j, glm::vec3& v) {
        j.at("x").get_to(v.x);
        j.at("y").get_to(v.y);
        j.at("z").get_to(v.z);
    }
};

} // namespace nlohmann

namespace ds_pba {

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

} // namespace ds_pba