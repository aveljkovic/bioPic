#include "biopic/index/hash_match_config.hpp"

#include "biopic/env.hpp"
#include "core/parse_double.hpp"

namespace biopic {

namespace {

using detail::parse_double;

} // namespace

std::optional<DistanceMetric> parse_distance_metric(std::string_view value) {
    if (value == "l1") {
        return DistanceMetric::L1;
    }
    if (value == "l2") {
        return DistanceMetric::L2;
    }
    if (value == "l2_squared" || value == "l2-squared") {
        return DistanceMetric::L2Squared;
    }
    return std::nullopt;
}

HashMatchConfig resolve_hash_match_config(const std::optional<double>& threshold_override,
                                          const std::optional<DistanceMetric>& metric_override) {
    HashMatchConfig config = kDefaultHashMatchConfig;

    if (const auto metric_from_env = read_env_variable("BIOPIC_SIMILARITY_METRIC");
        metric_from_env.has_value()) {
        if (const auto metric = parse_distance_metric(*metric_from_env); metric.has_value()) {
            config.metric = *metric;
        }
    }

    if (const auto threshold_from_env = read_env_variable("BIOPIC_SIMILARITY_THRESHOLD");
        threshold_from_env.has_value()) {
        if (const auto threshold = parse_double(*threshold_from_env); threshold.has_value()) {
            config.threshold = *threshold;
        }
    }

    if (metric_override.has_value()) {
        config.metric = *metric_override;
    }
    if (threshold_override.has_value()) {
        config.threshold = *threshold_override;
    }
    return config;
}

} // namespace biopic
