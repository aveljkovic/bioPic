#include "biopic/distance.hpp"

#include <cmath>
#include <cstdint>

namespace biopic {

std::int64_t l1_distance(const FingerprintBytes& a, const FingerprintBytes& b) {
    std::int64_t sum = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto diff = static_cast<std::int64_t>(a[i]) - static_cast<std::int64_t>(b[i]);
        sum += diff >= 0 ? diff : -diff;
    }
    return sum;
}

std::int64_t l2_squared_distance(const FingerprintBytes& a, const FingerprintBytes& b) {
    std::int64_t sum = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto diff = static_cast<std::int64_t>(a[i]) - static_cast<std::int64_t>(b[i]);
        sum += diff * diff;
    }
    return sum;
}

double l2_distance(const FingerprintBytes& a, const FingerprintBytes& b) {
    return std::sqrt(static_cast<double>(l2_squared_distance(a, b)));
}

DistanceResult compare_fingerprints(const Fingerprint& a, const Fingerprint& b,
                                    DistanceMetric metric) {
    DistanceResult result{.metric = metric, .value = 0.0};
    switch (metric) {
    case DistanceMetric::L1:
        result.value = static_cast<double>(l1_distance(a.bytes, b.bytes));
        break;
    case DistanceMetric::L2Squared:
        result.value = static_cast<double>(l2_squared_distance(a.bytes, b.bytes));
        break;
    case DistanceMetric::L2:
        result.value = l2_distance(a.bytes, b.bytes);
        break;
    }
    return result;
}

} // namespace biopic
