#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "biopic/ai/model_input.hpp"
#include "biopic/ai/output_mapping.hpp"
#include "biopic/ai/tensor_contract.hpp"

namespace biopic {

struct ModelMetadata {
    std::string name;
    std::string version;
    std::string description;
    ModelInputRequirements input_requirements;
    InferenceTensorContract tensor_contract = InferenceTensorContract::biopic_v1();
    OutputMapping output_mapping;
    std::unordered_map<std::string, std::string> attributes;
};

// Framework-neutral model descriptor backed by an ONNX file and manifest.
class Model {
  public:
    Model(std::string name, std::filesystem::path path, ModelMetadata metadata);

    [[nodiscard]] static Model placeholder(std::string name);
    [[nodiscard]] static std::optional<Model> load_from_path(const std::filesystem::path& path);
    [[nodiscard]] static std::optional<Model> load_from_manifest(
        const std::filesystem::path& model_path, const std::filesystem::path& manifest_path);

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    [[nodiscard]] const ModelInputRequirements& input_requirements() const noexcept {
        return metadata_.input_requirements;
    }
    [[nodiscard]] const ModelMetadata& metadata() const noexcept { return metadata_; }
    [[nodiscard]] const OutputMapping& output_mapping() const noexcept {
        return metadata_.output_mapping;
    }
    [[nodiscard]] bool is_loaded() const noexcept { return loaded_; }

    [[nodiscard]] bool load();
    void unload() noexcept;

  private:
    std::string name_;
    std::filesystem::path path_;
    ModelMetadata metadata_;
    bool loaded_ = false;
};

[[nodiscard]] std::optional<ModelMetadata> parse_model_manifest(
    const std::filesystem::path& manifest_path);

} // namespace biopic
