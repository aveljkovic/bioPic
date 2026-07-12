#include "biopic/ai/model.hpp"

#include <fstream>
#include <sstream>

namespace biopic {

namespace {

std::string stem_name(const std::filesystem::path& path) {
    if (path.has_stem() && !path.stem().empty()) {
        return path.stem().string();
    }
    return path.string();
}

} // namespace

Model::Model(std::string name, std::filesystem::path path, ModelInputRequirements input_requirements,
             ModelMetadata metadata)
    : name_(std::move(name)),
      path_(std::move(path)),
      input_requirements_(input_requirements),
      metadata_(std::move(metadata)) {}

Model Model::placeholder(std::string name) {
    ModelMetadata metadata;
    metadata.version = "0.0.0";
    metadata.description = "Placeholder model descriptor for Milestone 2 scaffolding";
    metadata.attributes["framework"] = "none";
    metadata.attributes["status"] = "dummy";

    return Model(std::move(name), std::filesystem::path{}, ModelInputRequirements{}, std::move(metadata));
}

std::optional<Model> Model::load_from_path(const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::exists(path, error) || error) {
        return std::nullopt;
    }

    ModelMetadata metadata;
    metadata.version = "0.0.0";
    metadata.description = "Model path validated; ONNX session loads at inference time";
    metadata.attributes["source_path"] = path.string();

    const std::filesystem::path sidecar = path.parent_path() / (path.stem().string() + ".meta.txt");
    if (std::filesystem::is_regular_file(sidecar, error) && !error) {
        std::ifstream input(sidecar);
        std::string line;
        while (std::getline(input, line)) {
            const auto separator = line.find('=');
            if (separator == std::string::npos) {
                continue;
            }
            const std::string key = line.substr(0, separator);
            const std::string value = line.substr(separator + 1);
            if (key == "version") {
                metadata.version = value;
            } else if (key == "description") {
                metadata.description = value;
            } else {
                metadata.attributes[key] = value;
            }
        }
    }

    return Model(stem_name(path), path, ModelInputRequirements{}, std::move(metadata));
}

bool Model::load() {
    if (path_.empty()) {
        loaded_ = true;
        return true;
    }

    std::error_code error;
    if (!std::filesystem::exists(path_, error) || error) {
        loaded_ = false;
        return false;
    }

    loaded_ = true;
    return true;
}

void Model::unload() noexcept { loaded_ = false; }

} // namespace biopic
