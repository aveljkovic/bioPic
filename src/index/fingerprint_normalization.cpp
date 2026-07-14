#include "biopic/index/fingerprint_normalization.hpp"

#include "biopic/distance.hpp"

#include <cmath>
#include <limits>

namespace biopic {

namespace {

double compute_distance(const Fingerprint& left, const Fingerprint& right,
                        const HashMatchConfig& config) {
    if (config.use_normalized) {
        const NormalizedFingerprint normalized_left = normalize_fingerprint(left);
        const NormalizedFingerprint normalized_right = normalize_fingerprint(right);
        return normalized_l2_distance(normalized_left, normalized_right);
    }

    const DistanceResult result = compare_fingerprints(left, right, config.metric);
    return result.value;
}

} // namespace

NormalizedFingerprint normalize_fingerprint(const Fingerprint& fingerprint) {
    NormalizedFingerprint normalized{};
    double sum_of_squares = 0.0;

    for (std::size_t index = 0; index < fingerprint.bytes.size(); ++index) {
        const double value = static_cast<double>(fingerprint.bytes[index]) / 255.0;
        normalized[index] = value;
        sum_of_squares += value * value;
    }

    if (sum_of_squares <= std::numeric_limits<double>::epsilon()) {
        return normalized;
    }

    const double inverse_length = 1.0 / std::sqrt(sum_of_squares);
    for (double& value : normalized) {
        value *= inverse_length;
    }
    return normalized;
}

double normalized_l2_distance(const NormalizedFingerprint& left,
                                const NormalizedFingerprint& right) {
    double sum_of_squares = 0.0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        const double difference = left[index] - right[index];
        sum_of_squares += difference * difference;
    }
    return std::sqrt(sum_of_squares);
}

double fingerprint_distance(const Fingerprint& left, const Fingerprint& right,
                            const HashMatchConfig& config) {
    return compute_distance(left, right, config);
}

bool is_within_similarity_threshold(double distance, const HashMatchConfig& config) {
    return config.threshold > 0.0 && distance <= config.threshold;
}

} // namespace biopic
