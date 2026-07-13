#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "biopic/ai/classification_result.hpp"
#include "biopic/ai/classifier.hpp"
#include "biopic/database/fingerprint_store.hpp"
#include "biopic/fingerprint.hpp"
#include "biopic/image.hpp"
#include "biopic/types.hpp"

namespace biopic {

enum class ClassifierAvailability {
    Skipped,
    Unavailable,
    Available,
};

enum class MatchStatus {
    NoMatch,
    ExactMatch,
    SimilarMatch,
};

struct ScanResult {
    Fingerprint fingerprint;
    std::optional<ClassificationResult> classification;
    ClassifierAvailability classifier_availability = ClassifierAvailability::Skipped;
    std::string classifier_status = "not configured";
    MatchStatus match_status = MatchStatus::NoMatch;
    std::optional<FingerprintRecord> matched_record;
    double nearest_distance = 0.0;
};

[[nodiscard]] std::optional<ScanResult> scan_file(
    const std::filesystem::path& image_path, const FingerprintStore* store = nullptr,
    const std::optional<std::filesystem::path>& config_path = std::nullopt);

[[nodiscard]] ScanResult scan(const ImageView& image, const FingerprintStore* store = nullptr,
                            const std::optional<std::filesystem::path>& config_path = std::nullopt);

[[nodiscard]] ScanResult scan(const ImageView& image, IClassifier& classifier,
                            const FingerprintStore* store = nullptr);

[[nodiscard]] ModerationDecision scan_decision(const ScanResult& result);
[[nodiscard]] const char* scan_decision_label(ModerationDecision decision);
[[nodiscard]] const char* match_status_label(MatchStatus status);

} // namespace biopic
