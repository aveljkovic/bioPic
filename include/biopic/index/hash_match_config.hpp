#pragma once

#include <optional>
#include <string_view>

#include "biopic/types.hpp"

namespace biopic {

[[nodiscard]] std::optional<DistanceMetric> parse_distance_metric(std::string_view value);

[[nodiscard]] HashMatchConfig resolve_hash_match_config(
    const std::optional<double>& threshold_override = std::nullopt,
    const std::optional<DistanceMetric>& metric_override = std::nullopt);

} // namespace biopic
