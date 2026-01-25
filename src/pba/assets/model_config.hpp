// pba/assets/model_config.hpp
#pragma once

#include "pba/engine/scene_types.hpp"

#include <expected>
#include <filesystem>
#include <json.hpp>
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

struct ModelConfig
{
    std::string name{};
    Transform transform{};
};

void to_json(nlohmann::json& j, const ModelConfig& c);
void from_json(const nlohmann::json& j, ModelConfig& c);

std::expected<ModelConfig, ModelConfigError>
load_or_create_model_config(const std::filesystem::path& model_dir);

}  // namespace ds_pba
