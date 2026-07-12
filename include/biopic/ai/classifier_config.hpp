#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include "biopic/ai/classifier.hpp"

namespace biopic {

// Runtime classifier configuration loaded from disk or environment.
struct ClassifierConfig {
    ClassifierKind kind = ClassifierKind::Nudity;
    std::filesystem::path model_path;
    std::filesystem::path manifest_path;

    [[nodiscard]] static std::optional<ClassifierConfig> load_from_file(
        const std::filesystem::path& config_path);
    [[nodiscard]] static std::optional<ClassifierConfig> from_environment();
};

[[nodiscard]] std::unique_ptr<IClassifier> create_classifier(const ClassifierConfig& config,
                                                             RuntimeOptions options = {});

} // namespace biopic
