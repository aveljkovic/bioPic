#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "biopic/ai/classification_result.hpp"
#include "biopic/ai/classifier.hpp"
#include "biopic/fingerprint.hpp"
#include "biopic/image.hpp"
#include "biopic/types.hpp"

namespace biopic {

enum class ClassifierAvailability {
    Skipped,
    Unavailable,
    Available,
};

struct ScanResult {
    Fingerprint fingerprint;
    std::optional<ClassificationResult> classification;
    ClassifierAvailability classifier_availability = ClassifierAvailability::Skipped;
    std::string classifier_status = "not configured";
};

[[nodiscard]] std::optional<ScanResult> scan_file(
    const std::filesystem::path& image_path,
    const std::optional<std::filesystem::path>& config_path = std::nullopt);

[[nodiscard]] ScanResult scan(const ImageView& image,
                            const std::optional<std::filesystem::path>& config_path = std::nullopt);

[[nodiscard]] ScanResult scan(const ImageView& image, IClassifier& classifier);

[[nodiscard]] ModerationDecision scan_decision(const ScanResult& result);
[[nodiscard]] const char* scan_decision_label(ModerationDecision decision);

} // namespace biopic
