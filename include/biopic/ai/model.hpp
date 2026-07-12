#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace biopic {

struct ModelInputRequirements {
    int width = 224;
    int height = 224;
    int channels = 3;
    double scale = 1.0 / 255.0;
    std::array<float, 3> mean{0.0f, 0.0f, 0.0f};
    std::array<float, 3> stddev{1.0f, 1.0f, 1.0f};
};

struct ModelMetadata {
    std::string version;
    std::string description;
    std::unordered_map<std::string, std::string> attributes;
};

// Framework-neutral model descriptor. Loading validates paths only in Milestone 2.
class Model {
  public:
    Model(std::string name, std::filesystem::path path, ModelInputRequirements input_requirements,
          ModelMetadata metadata);

    [[nodiscard]] static Model placeholder(std::string name);
    [[nodiscard]] static std::optional<Model> load_from_path(const std::filesystem::path& path);

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    [[nodiscard]] const ModelInputRequirements& input_requirements() const noexcept {
        return input_requirements_;
    }
    [[nodiscard]] const ModelMetadata& metadata() const noexcept { return metadata_; }
    [[nodiscard]] bool is_loaded() const noexcept { return loaded_; }

    [[nodiscard]] bool load();
    void unload() noexcept;

  private:
    std::string name_;
    std::filesystem::path path_;
    ModelInputRequirements input_requirements_;
    ModelMetadata metadata_;
    bool loaded_ = false;
};

} // namespace biopic
