// pba/scene_context.cpp
#include "scene_context.hpp"

#include "serialisation.hpp"  // IWYU pragma: keep

#include <fstream>
#include <nlohmann/json.hpp>

namespace ds_pba
{
bool save_scene_to_file(const SceneContext& scene, const fs::path& path)
{
    try
    {
        nlohmann::json j = scene;
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        if (!out.is_open())
        {
            return false;
        }
        out << j.dump(4) << "\n";
        return out.good();
    }
    catch (...)
    {
        return false;
    }
}

std::optional<SceneContext> load_scene_from_file(const fs::path& path)
{
    try
    {
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open())
        {
            return std::nullopt;
        }
        nlohmann::json j;
        in >> j;

        SceneContext scene = j.get<SceneContext>();
        return scene;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

}  // namespace ds_pba
