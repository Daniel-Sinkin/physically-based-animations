// pba/assets/model_config.hpp
#pragma once

#include "pba/scene/scene_types.hpp"

#include <filesystem>
#include <json.hpp>
#include <optional>
#include <string>

namespace ds_pba
{
enum class ModelConfigError
{
    ModelDirNotFound,
    NoGltfFileFound,
    ConfigReadError,
    ConfigParseError,
    ConfigWriteError,
};

[[nodiscard]] constexpr const char* to_string(ModelConfigError e) noexcept
{
    switch (e)
    {
        case ModelConfigError::ModelDirNotFound:
            return "ModelDirNotFound";
        case ModelConfigError::NoGltfFileFound:
            return "NoGltfFileFound";
        case ModelConfigError::ConfigReadError:
            return "ConfigReadError";
        case ModelConfigError::ConfigParseError:
            return "ConfigParseError";
        case ModelConfigError::ConfigWriteError:
            return "ConfigWriteError";
    }
    return "Unknown";
}

struct ModelConfig
{
    std::string name{};
    Transform transform{};
};

void to_json(nlohmann::json& j, const ModelConfig& c);
void from_json(const nlohmann::json& j, ModelConfig& c);

// Returns nullopt on any error (logs internally)
[[nodiscard]] std::optional<ModelConfig>
load_or_create_model_config(const std::filesystem::path& model_dir);

}  // namespace ds_pba
