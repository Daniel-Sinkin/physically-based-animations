// pba/scene_context.cpp
#include "scene_context.hpp"
#include "serialisation.hpp" // IWYU pragma: keep

#include <fstream>
#include <nlohmann/json.hpp>

namespace ds_pba {

SceneHotReloader::SceneHotReloader(fs::path p) : path(std::move(p)) {}

void SceneHotReloader::init_if_exists() {
    if (fs::exists(path)) {
        last_write_time = fs::last_write_time(path);
        initialized = true;
    }
}

bool SceneHotReloader::changed() {
    if (!fs::exists(path)) {
        return false;
    }
    const fs::file_time_type cur = fs::last_write_time(path);
    if (!initialized) {
        last_write_time = cur;
        initialized = true;
        return false;
    }
    if (cur != last_write_time) {
        last_write_time = cur;
        return true;
    }
    return false;
}

bool save_scene_to_file(const SceneContext &scene, const fs::path &path) {
    try {
        nlohmann::json j = scene;
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            return false;
        }
        out << j.dump(4) << "\n";
        return out.good();
    } catch (...) {
        return false;
    }
}

std::optional<SceneContext> load_scene_from_file(const fs::path &path) {
    try {
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open()) {
            return std::nullopt;
        }
        nlohmann::json j;
        in >> j;

        SceneContext scene = j.get<SceneContext>();
        return scene;
    } catch (...) {
        return std::nullopt;
    }
}

bool try_hot_reload_scene(SceneContext &scene_context, const fs::path &path) {
    auto loaded = load_scene_from_file(path);
    if (!loaded) {
        return false;
    }

    Camera current_cam = scene_context.camera;
    scene_context = std::move(*loaded);
    scene_context.camera = current_cam;

    if (scene_context.selected_index &&
        *scene_context.selected_index >= scene_context.cube_objects.size()) {
        scene_context.selected_index = std::nullopt;
    }
    return true;
}

} // namespace ds_pba