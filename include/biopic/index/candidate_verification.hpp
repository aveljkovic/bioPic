#pragma once

#include <vector>

#include "biopic/database/fingerprint_record.hpp"
#include "biopic/fingerprint.hpp"
#include "biopic/index/similarity_index.hpp"
#include "biopic/types.hpp"

namespace biopic {

[[nodiscard]] NearestMatchResult verify_nearest_among_candidates(
    const Fingerprint& fingerprint, const std::vector<FingerprintRecord>& candidates,
    const HashMatchConfig& config);

[[nodiscard]] std::vector<SimilarityQueryResult> verify_similar_among_candidates(
    const Fingerprint& fingerprint, const std::vector<FingerprintRecord>& candidates,
    const HashMatchConfig& config, std::size_t limit = 0);

} // namespace biopic
