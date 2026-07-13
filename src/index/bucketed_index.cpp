#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "biopic/index/bucketed_index.hpp"

#include "biopic/database/fingerprint_store.hpp"
#include "biopic/index/candidate_verification.hpp"
#include "biopic/index/fingerprint_bloom_filter.hpp"
#include "biopic/index/fingerprint_bucket.hpp"

#include <unordered_map>
#include <vector>

namespace biopic {

struct BucketedIndexImpl {
    std::vector<FingerprintRecord> records;
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> buckets;
    FingerprintBloomFilter exact_bloom;
};

namespace {

std::vector<FingerprintRecord> collect_candidates(const BucketedIndexImpl& impl,
                                                  const Fingerprint& fingerprint,
                                                  const HashMatchConfig& config,
                                                  bool for_nearest) {
    std::vector<FingerprintRecord> candidates;
    if (impl.records.empty()) {
        return candidates;
    }

    std::vector<std::size_t> seen;
    seen.reserve(512);

    for (const BandBucketRef& ref : candidate_bucket_refs(fingerprint, config, for_nearest)) {
        const auto iterator =
            impl.buckets.find(bucket_lookup_key(ref.band_index, ref.bucket_key));
        if (iterator == impl.buckets.end()) {
            continue;
        }

        for (const std::size_t record_index : iterator->second) {
            bool already_seen = false;
            for (const std::size_t prior_index : seen) {
                if (prior_index == record_index) {
                    already_seen = true;
                    break;
                }
            }
            if (already_seen) {
                continue;
            }
            seen.push_back(record_index);
            candidates.push_back(impl.records[record_index]);
        }
    }

    if (for_nearest && candidates.empty()) {
        return impl.records;
    }
    return candidates;
}

} // namespace

BucketedIndex::BucketedIndex() : impl_(std::make_unique<BucketedIndexImpl>()) {
    impl_->exact_bloom.reset(1024);
}

BucketedIndex::~BucketedIndex() = default;

bool BucketedIndex::add(const FingerprintRecord& candidate) {
    if (candidate.label.empty()) {
        return false;
    }

    const std::size_t record_index = impl_->records.size();
    impl_->records.push_back(candidate);
    impl_->exact_bloom.add(candidate.fingerprint);

    for (const BandBucketRef& ref : band_bucket_refs(candidate.fingerprint)) {
        impl_->buckets[bucket_lookup_key(ref.band_index, ref.bucket_key)].push_back(record_index);
    }
    return true;
}

std::vector<SimilarityQueryResult> BucketedIndex::query(const Fingerprint& fingerprint,
                                                        const HashMatchConfig& config,
                                                        std::size_t limit) const {
    return verify_similar_among_candidates(fingerprint, impl_->records, config, limit);
}

NearestMatchResult BucketedIndex::find_nearest(const Fingerprint& fingerprint,
                                             const HashMatchConfig& config) const {
    if (impl_->records.empty()) {
        return {};
    }

    if (impl_->exact_bloom.might_contain(fingerprint)) {
        for (const FingerprintRecord& candidate : impl_->records) {
            if (fingerprints_equal(candidate.fingerprint, fingerprint)) {
                NearestMatchResult result;
                result.exact_match = true;
                result.distance = 0.0;
                result.record = candidate;
                return result;
            }
        }
    }

    const std::vector<FingerprintRecord> candidates =
        collect_candidates(*impl_, fingerprint, config, true);
    return verify_nearest_among_candidates(fingerprint, candidates, config);
}

std::size_t BucketedIndex::size() const noexcept {
    return impl_->records.size();
}

std::size_t BucketedIndex::count_query_candidates(const Fingerprint& fingerprint,
                                                const HashMatchConfig& config,
                                                bool for_nearest) const {
    return collect_candidates(*impl_, fingerprint, config, for_nearest).size();
}

std::unique_ptr<SimilarityIndex> create_bucketed_index() {
    return std::make_unique<BucketedIndex>();
}

} // namespace biopic
