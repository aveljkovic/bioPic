#include "pipeline/scanner.hpp"

#include "biopic/ai/classifier_config.hpp"
#include "biopic/hasher.hpp"
#include "biopic/image.hpp"
#include "biopic/index/fingerprint_normalization.hpp"
#include "biopic/policy/moderation_policy.hpp"

namespace biopic {

namespace {

void apply_database_lookup(ScanResult& result, const FingerprintStore* store,
                           const HashMatchConfig& match_config) {
    if (store == nullptr) {
        result.match_status = MatchStatus::NoMatch;
        return;
    }

    const NearestMatchResult nearest = store->find_nearest(result.fingerprint, match_config);
    result.nearest_distance = nearest.distance;

    if (nearest.exact_match && nearest.record.has_value()) {
        result.match_status = MatchStatus::ExactMatch;
        result.matched_record = nearest.record;
        return;
    }

    if (nearest.record.has_value() &&
        is_within_similarity_threshold(nearest.distance, match_config)) {
        result.match_status = MatchStatus::SimilarMatch;
        result.matched_record = nearest.record;
        return;
    }

    result.match_status = MatchStatus::NoMatch;
}

ScanResult scan_with_optional_classifier(const ImageView& image,
                                        const std::optional<std::filesystem::path>& config_path,
                                        const FingerprintStore* store,
                                        const HashMatchConfig& match_config) {
    Hasher hasher;
    ScanResult result;
    result.fingerprint = hasher.compute(image);
    apply_database_lookup(result, store, match_config);

    if (result.match_status == MatchStatus::ExactMatch) {
        result.classifier_availability = ClassifierAvailability::Skipped;
        result.classifier_status = "skipped (database match)";
        return result;
    }

    if (!config_path.has_value()) {
        result.classifier_availability = ClassifierAvailability::Skipped;
        result.classifier_status = "not configured";
        return result;
    }

    const auto config = ClassifierConfig::load_from_file(*config_path);
    if (!config.has_value()) {
        result.classifier_availability = ClassifierAvailability::Unavailable;
        result.classifier_status = "configuration invalid";
        return result;
    }

    const auto classifier = create_classifier(*config);
    if (classifier == nullptr) {
        result.classifier_availability = ClassifierAvailability::Unavailable;
        result.classifier_status = "initialization failed";
        return result;
    }

    result.classifier_availability = ClassifierAvailability::Available;
    result.classifier_status = "available";
    result.classification = classifier->classify(image);
    return result;
}

} // namespace

std::optional<ScanResult> scan_file(const std::filesystem::path& image_path,
                                    const FingerprintStore* store,
                                    const std::optional<std::filesystem::path>& config_path,
                                    const HashMatchConfig& match_config) {
    ImageDecoder decoder;
    const auto decoded = decoder.decode_file(image_path.string());
    if (!decoded.has_value()) {
        return std::nullopt;
    }

    const ImageView view(decoded->width, decoded->height, decoded->rgb);
    return scan(view, store, config_path, match_config);
}

ScanResult scan(const ImageView& image, const FingerprintStore* store,
                const std::optional<std::filesystem::path>& config_path,
                const HashMatchConfig& match_config) {
    return scan_with_optional_classifier(image, config_path, store, match_config);
}

ScanResult scan(const ImageView& image, IClassifier& classifier, const FingerprintStore* store,
                const HashMatchConfig& match_config) {
    Hasher hasher;
    ScanResult result;
    result.fingerprint = hasher.compute(image);
    apply_database_lookup(result, store, match_config);

    if (result.match_status == MatchStatus::ExactMatch) {
        result.classifier_availability = ClassifierAvailability::Skipped;
        result.classifier_status = "skipped (database match)";
        return result;
    }

    result.classifier_availability = ClassifierAvailability::Available;
    result.classifier_status = "available";
    result.classification = classifier.classify(image);
    return result;
}

const char* scan_decision_label(ModerationDecision decision) {
    switch (decision) {
    case ModerationDecision::Allow:
        return "ALLOW";
    case ModerationDecision::Review:
        return "REVIEW";
    case ModerationDecision::Block:
        return "BLOCK";
    }
    return "UNKNOWN";
}

const char* match_status_label(MatchStatus status) {
    switch (status) {
    case MatchStatus::NoMatch:
        return "no match";
    case MatchStatus::ExactMatch:
        return "exact match";
    case MatchStatus::SimilarMatch:
        return "similar match";
    }
    return "unknown";
}

} // namespace biopic
