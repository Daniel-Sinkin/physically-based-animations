// pba/assets/model_config.hpp
#pragma once

#include "pba/scene/world_types.hpp"

#include <filesystem>
#include <gsl/string_span>
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

[[nodiscard]] constexpr auto to_string(ModelConfigError e) noexcept -> czstring
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

auto to_json(nlohmann::json& j, const ModelConfig& c) -> void;
auto from_json(const nlohmann::json& j, ModelConfig& c) -> void;

// Returns nullopt on any error (logs internally)
[[nodiscard]] auto load_or_create_model_config(const std::filesystem::path& model_dir)
    -> std::optional<ModelConfig>;

}  // namespace ds_pba
