// pba/serialisation.cpp
#include "serialisation.hpp"

#include <optional>
#include <vector>

namespace ds_pba {

using json = nlohmann::json;

void to_json(json& j, const Transform& t) {
    j = json{
        {"position", t.position},
        {"rotation_deg", t.rotation_deg},
        {"scale", t.scale},
    };
}

void from_json(const json& j, Transform& t) {
    j.at("position").get_to(t.position);
    j.at("rotation_deg").get_to(t.rotation_deg);
    j.at("scale").get_to(t.scale);
}

void to_json(json& j, const Object& o) {
    j = json{
        {"name", o.name},
        {"transform", o.transform},
        {"color", o.color},
    };
}

void from_json(const json& j, Object& o) {
    j.at("name").get_to(o.name);
    j.at("transform").get_to(o.transform);
    j.at("color").get_to(o.color);
}

void to_json(json& j, const Camera& c) {
    j = json{
        {"pivot", c.pivot},
        {"distance", c.distance},
        {"yaw", c.yaw},
        {"pitch", c.pitch},
        {"fov_y", c.fov_y},
        {"z_near", c.z_near},
        {"z_far", c.z_far},
    };
}

void from_json(const json& j, Camera& c) {
    j.at("pivot").get_to(c.pivot);
    j.at("distance").get_to(c.distance);
    j.at("yaw").get_to(c.yaw);
    j.at("pitch").get_to(c.pitch);
    j.at("fov_y").get_to(c.fov_y);
    j.at("z_near").get_to(c.z_near);
    j.at("z_far").get_to(c.z_far);
}

void to_json(json& j, const SceneContext& s) {
    j = json{
        {"camera", s.camera},
        {"cube_objects", s.cube_objects},
        {"selected_index", s.selected_index ? json(*s.selected_index) : json(nullptr)},
    };
}

void from_json(const json& j, SceneContext& s) {
    j.at("camera").get_to(s.camera);
    j.at("cube_objects").get_to(s.cube_objects);

    auto it = j.find("selected_index");
    if (it == j.end() || it->is_null()) {
        s.selected_index = std::nullopt;
    } else {
        s.selected_index = it->get<usize>();
    }

    if (s.selected_index && *s.selected_index >= s.cube_objects.size()) {
        s.selected_index = std::nullopt;
    }
}

} // namespace ds_pba