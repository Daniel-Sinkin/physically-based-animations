// pba/model_config.cpp
#include "pba/pch.hpp"  // IWYU pragma: keep
//
#include "pba/model_config.hpp"
#include "pba/serialization.hpp"
//

#include <json.hpp>

namespace ds_pba
{

void to_json(nlohmann::json& j, const ModelConfig& c)
{
    j = nlohmann::json::object();
    j["name"] = c.name;
    j["transform"] = c.transform;
}

void from_json(const nlohmann::json& j, ModelConfig& c)
{
    c.name = j.at("name").get<std::string>();
    c.transform = j.at("transform").get<Transform>();
}

namespace
{

static std::filesystem::path config_path(const std::filesystem::path& model_dir)
{
    return model_dir / "config.json";
}

static std::expected<nlohmann::json, ModelConfigError>
read_json_file(const std::filesystem::path& p)
{
    std::ifstream f(p);
    if (!f.is_open())
    {
        return std::unexpected(ModelConfigError::ConfigReadError);
    }

    try
    {
        nlohmann::json j;
        f >> j;
        return j;
    }
    catch (...)
    {
        return std::unexpected(ModelConfigError::ConfigParseError);
    }
}

static std::expected<void, ModelConfigError>
write_json_file(const std::filesystem::path& p, const nlohmann::json& j)
{
    std::ofstream f(p);
    if (!f.is_open())
    {
        return std::unexpected(ModelConfigError::ConfigWriteError);
    }

    f << j.dump(4) << "\n";
    if (f.fail())
    {
        return std::unexpected(ModelConfigError::ConfigWriteError);
    }
    return {};
}

static std::string model_name_from_dir(const std::filesystem::path& model_dir)
{
    return model_dir.filename().string();
}

}  // namespace

std::expected<ModelConfig, ModelConfigError>
load_or_create_model_config(const std::filesystem::path& model_dir)
{
    if (!std::filesystem::exists(model_dir) || !std::filesystem::is_directory(model_dir))
    {
        return std::unexpected(ModelConfigError::ModelDirNotFound);
    }

    const auto cfg_path = config_path(model_dir);
    const auto model_name = model_name_from_dir(model_dir);

    if (!std::filesystem::exists(cfg_path))
    {
        ModelConfig cfg{};
        cfg.name = model_name;
        cfg.transform = Transform{};  // defaults: pos=0 rot=0 scale=1

        nlohmann::json j = cfg;
        auto wr = write_json_file(cfg_path, j);
        if (!wr)
        {
            return std::unexpected(wr.error());
        }
        return cfg;
    }

    auto jr = read_json_file(cfg_path);
    if (!jr)
    {
        return std::unexpected(jr.error());
    }

    try
    {
        ModelConfig cfg = jr->get<ModelConfig>();

        // Keep consistent with folder name (directory is source of truth)
        cfg.name = model_name;

        return cfg;
    }
    catch (...)
    {
        return std::unexpected(ModelConfigError::ConfigParseError);
    }
}

}  // namespace ds_pba
