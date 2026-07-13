#include "pipeline/scanner.hpp"

#include "biopic/ai/classifier_config.hpp"
#include "biopic/hasher.hpp"
#include "biopic/image.hpp"

namespace biopic {

namespace {

ScanResult scan_with_optional_classifier(const ImageView& image,
                                        const std::optional<std::filesystem::path>& config_path) {
    Hasher hasher;
    ScanResult result;
    result.fingerprint = hasher.compute(image);

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
                                    const std::optional<std::filesystem::path>& config_path) {
    ImageDecoder decoder;
    const auto decoded = decoder.decode_file(image_path.string());
    if (!decoded.has_value()) {
        return std::nullopt;
    }

    const ImageView view(decoded->width, decoded->height, decoded->rgb);
    return scan(view, config_path);
}

ScanResult scan(const ImageView& image,
                const std::optional<std::filesystem::path>& config_path) {
    return scan_with_optional_classifier(image, config_path);
}

ScanResult scan(const ImageView& image, IClassifier& classifier) {
    Hasher hasher;
    ScanResult result;
    result.fingerprint = hasher.compute(image);
    result.classifier_availability = ClassifierAvailability::Available;
    result.classifier_status = "available";
    result.classification = classifier.classify(image);
    return result;
}

ModerationDecision scan_decision(const ScanResult& result) {
    if (!result.classification.has_value()) {
        return ModerationDecision::Allow;
    }
    return result.classification->detected ? ModerationDecision::Block : ModerationDecision::Allow;
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

} // namespace biopic
