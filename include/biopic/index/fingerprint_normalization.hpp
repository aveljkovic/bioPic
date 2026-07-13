#pragma once

#include <array>

#include "biopic/fingerprint.hpp"
#include "biopic/types.hpp"

namespace biopic {

using NormalizedFingerprint = std::array<double, kFingerprintComponentCount>;

[[nodiscard]] NormalizedFingerprint normalize_fingerprint(const Fingerprint& fingerprint);

[[nodiscard]] double normalized_l2_distance(const NormalizedFingerprint& left,
                                            const NormalizedFingerprint& right);

[[nodiscard]] double fingerprint_distance(const Fingerprint& left, const Fingerprint& right,
                                          const HashMatchConfig& config);

[[nodiscard]] bool is_within_similarity_threshold(double distance, const HashMatchConfig& config);

} // namespace biopic
