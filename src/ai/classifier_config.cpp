#include "biopic/ai/classifier_config.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
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

std::optional<ClassifierKind> parse_classifier_kind(const std::string& value) {
    if (value == "nudity") {
        return ClassifierKind::Nudity;
    }
    if (value == "violence") {
        return ClassifierKind::Violence;
    }
    if (value == "weapons") {
        return ClassifierKind::Weapons;
    }
    if (value == "drugs") {
        return ClassifierKind::Drugs;
    }
    if (value == "child_safety") {
        return ClassifierKind::ChildSafety;
    }
    return std::nullopt;
}

std::filesystem::path resolve_relative_path(const std::filesystem::path& base,
                                            const std::filesystem::path& candidate) {
    if (candidate.empty() || candidate.is_absolute()) {
        return candidate;
    }
    return base / candidate;
}

bool apply_config_value(ClassifierConfig& config, const std::filesystem::path& config_dir,
                        const std::string& key, const std::string& value) {
    if (key == "model_path") {
        config.model_path = resolve_relative_path(config_dir, value);
        return true;
    }
    if (key == "manifest_path") {
        config.manifest_path = resolve_relative_path(config_dir, value);
        return true;
    }
    if (key == "kind") {
        const auto kind = parse_classifier_kind(value);
        if (!kind.has_value()) {
            return false;
        }
        config.kind = *kind;
        return true;
    }
    return false;
}

std::optional<ClassifierConfig> parse_config_stream(std::istream& input,
                                                    const std::filesystem::path& config_path) {
    ClassifierConfig config;
    const std::filesystem::path config_dir = config_path.parent_path();
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
        if (!apply_config_value(config, config_dir, key, value)) {
            return std::nullopt;
        }
    }

    if (config.model_path.empty()) {
        return std::nullopt;
    }
    if (config.manifest_path.empty()) {
        config.manifest_path = config.model_path.parent_path() /
                               (config.model_path.stem().string() + ".biopic.manifest");
    }
    return config;
}

} // namespace

std::optional<ClassifierConfig> ClassifierConfig::load_from_file(
    const std::filesystem::path& config_path) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(config_path, error) || error) {
        return std::nullopt;
    }

    std::ifstream input(config_path);
    if (!input) {
        return std::nullopt;
    }
    return parse_config_stream(input, config_path);
}

std::optional<ClassifierConfig> ClassifierConfig::from_environment() {
    const char* config_path = std::getenv("BIOPIC_CLASSIFIER_CONFIG");
    if (config_path == nullptr || config_path[0] == '\0') {
        return std::nullopt;
    }
    return load_from_file(config_path);
}

std::unique_ptr<IClassifier> create_classifier(const ClassifierConfig& config,
                                               RuntimeOptions options) {
    const auto model = Model::load_from_manifest(config.model_path, config.manifest_path);
    if (!model.has_value()) {
        return nullptr;
    }
    return std::make_unique<OnnxClassifier>(config.kind, *model, options);
}

} // namespace biopic
