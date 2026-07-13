#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace biopic {

constexpr std::uint32_t kFingerprintComponentCount = 144;
constexpr std::uint32_t kHashAlgorithmVersion = 1;

enum class HashAlgorithm : std::uint8_t { BioPic = 1 };

enum class DistanceMetric : std::uint8_t { L1, L2, L2Squared };

enum class ModerationDecision : std::uint8_t {
    Allow,
    Review,
    Block,
};

enum class ModerationReason : std::uint8_t {
    None,
    KnownHashMatch,
    ModelClassification,
    InvalidInput,
    ProcessingError,
};

struct DecodeLimits {
    std::size_t max_encoded_bytes = 20 * 1024 * 1024;
    int max_width = 8192;
    int max_height = 8192;
    std::uint64_t max_decoded_pixels = 64 * 1024 * 1024;
    int decode_timeout_ms = 5000;
};

struct HashMatchConfig {
    DistanceMetric metric = DistanceMetric::L2;
    double threshold = 10.0;
    bool use_normalized = false;
    std::size_t max_results = 0;
};

constexpr HashMatchConfig kDefaultHashMatchConfig{};

struct ClassificationScores {
    double safe = 0.0;
    double suggestive = 0.0;
    double partial_nudity = 0.0;
    double explicit_nudity = 0.0;
    double uncertain = 0.0;
};

} // namespace biopic
