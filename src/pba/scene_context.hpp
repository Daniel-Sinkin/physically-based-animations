// pba/scene_context.hpp
#pragma once

#include "camera.hpp"
#include "pba/scene_types.hpp"
#include "pba/types.hpp"  // IWYU pragma: keep

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace ds_pba
{
namespace fs = std::filesystem;
struct SceneHotReloader
{
    fs::path path{};
    fs::file_time_type last_write_time{};
    bool initialized{false};

    explicit SceneHotReloader(fs::path p);

    void init_if_exists();
    [[nodiscard]] bool changed();
};

struct SceneContext
{
    Camera camera{};
    std::vector<Object> cube_objects{};
    std::vector<Object> sphere_objects{};
    std::optional<usize> selected_index{};
    std::optional<ObjectType> selected_type{};
    std::unique_ptr<SceneHotReloader> reloader;
};

inline constexpr const char* k_scene_path = "scene.json";

[[nodiscard]] bool save_scene_to_file(const SceneContext& scene, const fs::path& path);
[[nodiscard]] std::optional<SceneContext> load_scene_from_file(const fs::path& path);
[[nodiscard]] bool try_hot_reload_scene(SceneContext& scene_context, const fs::path& path);

}  // namespace ds_pba
