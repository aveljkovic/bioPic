#pragma once

#include "biopic/fingerprint.hpp"
#include "biopic/types.hpp"

namespace biopic {

struct DistanceResult {
    DistanceMetric metric = DistanceMetric::L1;
    double value = 0.0;
};

[[nodiscard]] DistanceResult compare_fingerprints(const Fingerprint& a, const Fingerprint& b,
                                                  DistanceMetric metric);

[[nodiscard]] std::int64_t l1_distance(const FingerprintBytes& a, const FingerprintBytes& b);

[[nodiscard]] std::int64_t l2_squared_distance(const FingerprintBytes& a, const FingerprintBytes& b);

[[nodiscard]] double l2_distance(const FingerprintBytes& a, const FingerprintBytes& b);

} // namespace biopic
