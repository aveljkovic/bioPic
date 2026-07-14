#include "biopic/index/candidate_verification.hpp"

#include "biopic/database/fingerprint_store.hpp"
#include "biopic/index/fingerprint_normalization.hpp"

#include <algorithm>
#include <limits>

namespace biopic {

namespace {

struct RankedRecord {
    FingerprintRecord matched;
    double distance = 0.0;
};

std::size_t effective_limit(std::size_t limit, const HashMatchConfig& config) {
    if (limit == 0) {
        return config.max_results;
    }
    if (config.max_results == 0) {
        return limit;
    }
    return std::min(limit, config.max_results);
}

} // namespace

NearestMatchResult verify_nearest_among_candidates(
    const Fingerprint& fingerprint, const std::vector<FingerprintRecord>& candidates,
    const HashMatchConfig& config) {
    NearestMatchResult result;
    if (candidates.empty()) {
        return result;
    }

    for (const FingerprintRecord& candidate : candidates) {
        if (fingerprints_equal(candidate.fingerprint, fingerprint)) {
            result.exact_match = true;
            result.distance = 0.0;
            result.record = candidate;
            return result;
        }
    }

    double best_distance = std::numeric_limits<double>::infinity();
    std::optional<FingerprintRecord> best_match;
    for (const FingerprintRecord& candidate : candidates) {
        const double distance = fingerprint_distance(fingerprint, candidate.fingerprint, config);
        if (distance < best_distance) {
            best_distance = distance;
            best_match = candidate;
        }
    }

    result.exact_match = false;
    result.distance = best_distance;
    result.record = best_match;
    return result;
}

std::vector<SimilarityQueryResult> verify_similar_among_candidates(
    const Fingerprint& fingerprint, const std::vector<FingerprintRecord>& candidates,
    const HashMatchConfig& config, std::size_t limit) {
    std::vector<RankedRecord> ranked;
    ranked.reserve(candidates.size());

    for (const FingerprintRecord& candidate : candidates) {
        if (fingerprints_equal(candidate.fingerprint, fingerprint)) {
            continue;
        }

        const double distance = fingerprint_distance(fingerprint, candidate.fingerprint, config);
        if (is_within_similarity_threshold(distance, config)) {
            ranked.push_back(RankedRecord{candidate, distance});
        }
    }

    std::sort(ranked.begin(), ranked.end(),
              [](const RankedRecord& left, const RankedRecord& right) {
                  return left.distance < right.distance;
              });

    const std::size_t result_limit = effective_limit(limit, config);
    if (result_limit > 0 && ranked.size() > result_limit) {
        ranked.resize(result_limit);
    }

    std::vector<SimilarityQueryResult> results;
    results.reserve(ranked.size());
    for (const RankedRecord& ranked_entry : ranked) {
        results.push_back(SimilarityQueryResult{ranked_entry.matched, ranked_entry.distance});
    }
    return results;
}

} // namespace biopic
