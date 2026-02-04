// pba/assets/model_config.cpp
#include "pba/core/pch.hpp"  // IWYU pragma: keep
//
#include "pba/assets/model_config.hpp"
#include "pba/assets/serialization.hpp"  // IWYU pragma: keep
#include "pba/scene/world_types.hpp"
//

#include <json.hpp>

namespace ds_pba
{
auto to_json(nlohmann::json& j, const ModelConfig& c) -> void
{
    j = nlohmann::json::object();
    j["name"] = c.name;
    j["transform"] = c.transform;
}

auto from_json(const nlohmann::json& j, ModelConfig& c) -> void
{
    c.name = j.at("name").get<std::string>();
    c.transform = j.at("transform").get<Transform>();
}

namespace
{
std::filesystem::path config_path(const std::filesystem::path& model_dir)
{
    return model_dir / "config.json";
}

std::optional<nlohmann::json> read_json_file(const std::filesystem::path& p)
{
    std::ifstream f(p);
    if (!f.is_open())
    {
        std::println(stderr, "[ModelConfig] Failed to open '{}'", p.string());
        return std::nullopt;
    }

    nlohmann::json j = nlohmann::json::parse(f, nullptr, false);
    if (j.is_discarded())
    {
        std::println(stderr, "[ModelConfig] JSON parse error in '{}'", p.string());
        return std::nullopt;
    }

    return j;
}

auto write_json_file(const std::filesystem::path& p, const nlohmann::json& j) -> bool
{
    std::ofstream f(p);
    if (!f.is_open())
    {
        std::println(stderr, "[ModelConfig] Failed to write '{}'", p.string());
        return false;
    }

    try
    {
        f << j.dump(4) << "\n";
    }
    catch (const std::exception& e)
    {
        std::println(stderr, "[ModelConfig] JSON dump failed for '{}': {}", p.string(), e.what());
        return false;
    }
    catch (...)
    {
        std::println(
            stderr, "[ModelConfig] JSON dump failed for '{}': (unknown error)", p.string()
        );
        return false;
    }

    if (f.fail())
    {
        std::println(stderr, "[ModelConfig] Write failed for '{}'", p.string());
        return false;
    }
    return true;
}

auto model_name_from_dir(const std::filesystem::path& model_dir) -> std::string
{
    return model_dir.filename().string();
}

}  // namespace

auto load_or_create_model_config(const std::filesystem::path& model_dir)
    -> std::optional<ModelConfig>
{
    namespace fs = std::filesystem;

    std::error_code ec{};
    const bool exists = fs::exists(model_dir, ec);
    if (ec || !exists || !fs::is_directory(model_dir, ec) || ec)
    {
        std::println(stderr, "[ModelConfig] Model directory not found: '{}'", model_dir.string());
        return std::nullopt;
    }

    const fs::path cfg_path{config_path(model_dir)};
    const std::string model_name{model_name_from_dir(model_dir)};

    if (!fs::exists(cfg_path, ec) || ec)
    {
        ModelConfig cfg{};
        cfg.name = model_name;
        cfg.transform = Transform{};

        const nlohmann::json j{cfg};
        if (!write_json_file(cfg_path, j))
        {
            std::println(stderr, "[ModelConfig] Failed to create config: '{}'", cfg_path.string());
            return std::nullopt;
        }
        return cfg;
    }

    auto jr = read_json_file(cfg_path);
    if (!jr)
    {
        return std::nullopt;
    }

    try
    {
        ModelConfig cfg = jr->get<ModelConfig>();
        cfg.name = model_name;
        return cfg;
    }
    catch (const std::exception& e)
    {
        std::println(stderr, "[ModelConfig] Invalid config '{}': {}", cfg_path.string(), e.what());
        return std::nullopt;
    }
    catch (...)
    {
        std::println(
            stderr, "[ModelConfig] Invalid config '{}': (unknown error)", cfg_path.string()
        );
        return std::nullopt;
    }
}

}  // namespace ds_pba
