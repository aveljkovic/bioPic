#include "biopic/ai/model.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace biopic {

namespace {

std::string trim(std::string value) {
    const auto not_space = [](unsigned char character) { return !std::isspace(character); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::vector<std::string> split_csv(const std::string& value) {
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, ',')) {
        part = trim(part);
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

bool parse_double(const std::string& value, double& out) {
    try {
        std::size_t consumed = 0;
        out = std::stod(value, &consumed);
        return consumed == value.size();
    } catch (const std::exception&) {
        return false;
    }
}

bool parse_int(const std::string& value, int& out) {
    try {
        std::size_t consumed = 0;
        out = std::stoi(value, &consumed);
        return consumed == value.size();
    } catch (const std::exception&) {
        return false;
    }
}

std::string stem_name(const std::filesystem::path& path) {
    if (path.has_stem() && !path.stem().empty()) {
        return path.stem().string();
    }
    return path.string();
}

void apply_manifest_value(ModelMetadata& metadata, const std::string& key,
                          const std::string& value) {
    if (key == "name") {
        metadata.name = value;
    } else if (key == "version") {
        metadata.version = value;
    } else if (key == "description") {
        metadata.description = value;
    } else if (key == "input_width") {
        parse_int(value, metadata.input_requirements.width);
    } else if (key == "input_height") {
        parse_int(value, metadata.input_requirements.height);
    } else if (key == "input_channels") {
        parse_int(value, metadata.input_requirements.channels);
    } else if (key == "scale") {
        parse_double(value, metadata.input_requirements.scale);
    } else if (key == "mean_r") {
        metadata.input_requirements.mean[0] = std::stof(value);
    } else if (key == "mean_g") {
        metadata.input_requirements.mean[1] = std::stof(value);
    } else if (key == "mean_b") {
        metadata.input_requirements.mean[2] = std::stof(value);
    } else if (key == "std_r") {
        metadata.input_requirements.stddev[0] = std::stof(value);
    } else if (key == "std_g") {
        metadata.input_requirements.stddev[1] = std::stof(value);
    } else if (key == "std_b") {
        metadata.input_requirements.stddev[2] = std::stof(value);
    } else if (key == "labels") {
        metadata.output_mapping.labels = split_csv(value);
    } else if (key == "detection_label") {
        metadata.output_mapping.detection_label = value;
    } else if (key == "detection_threshold") {
        parse_double(value, metadata.output_mapping.detection_threshold);
    } else {
        metadata.attributes[key] = value;
    }
}

} // namespace

std::optional<ModelMetadata> parse_model_manifest(const std::filesystem::path& manifest_path) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(manifest_path, error) || error) {
        return std::nullopt;
    }

    std::ifstream input(manifest_path);
    if (!input) {
        return std::nullopt;
    }

    ModelMetadata metadata;
    metadata.tensor_contract = InferenceTensorContract::biopic_v1();
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        apply_manifest_value(metadata, key, value);
    }

    if (metadata.name.empty()) {
        metadata.name = stem_name(manifest_path);
    }
    if (metadata.version.empty()) {
        metadata.version = "0.0.0";
    }
    return metadata;
}

Model::Model(std::string name, std::filesystem::path path, ModelMetadata metadata)
    : name_(std::move(name)), path_(std::move(path)), metadata_(std::move(metadata)) {
    if (metadata_.name.empty()) {
        metadata_.name = name_;
    }
}

Model Model::placeholder(std::string name) {
    ModelMetadata metadata;
    metadata.name = name;
    metadata.version = "0.0.0";
    metadata.description = "Placeholder model descriptor for Milestone 2 scaffolding";
    metadata.attributes["framework"] = "none";
    metadata.attributes["status"] = "dummy";
    metadata.output_mapping.labels = {"safe"};
    metadata.output_mapping.detection_label = "unsafe";
    return Model(std::move(name), std::filesystem::path{}, std::move(metadata));
}

std::optional<Model> Model::load_from_path(const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::exists(path, error) || error) {
        return std::nullopt;
    }

    const std::filesystem::path manifest_path =
        path.parent_path() / (path.stem().string() + ".biopic.manifest");
    if (std::filesystem::is_regular_file(manifest_path, error) && !error) {
        return load_from_manifest(path, manifest_path);
    }

    ModelMetadata metadata;
    metadata.name = stem_name(path);
    metadata.version = "0.0.0";
    metadata.description = "Model path validated; manifest sidecar not found";
    metadata.attributes["source_path"] = path.string();
    return Model(metadata.name, path, std::move(metadata));
}

std::optional<Model> Model::load_from_manifest(const std::filesystem::path& model_path,
                                             const std::filesystem::path& manifest_path) {
    std::error_code error;
    if (!std::filesystem::exists(model_path, error) || error) {
        return std::nullopt;
    }

    const auto metadata = parse_model_manifest(manifest_path);
    if (!metadata.has_value()) {
        return std::nullopt;
    }

    const std::string model_name =
        metadata->name.empty() ? stem_name(model_path) : metadata->name;
    return Model(model_name, model_path, *metadata);
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
