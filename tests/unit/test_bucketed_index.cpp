#include <gtest/gtest.h>

#include "biopic/index/bucketed_index.hpp"
#include "biopic/index/fingerprint_bucket.hpp"
#include "biopic/index/similarity_index.hpp"

namespace {

biopic::Fingerprint make_fingerprint(std::uint8_t seed) {
    biopic::Fingerprint fingerprint;
    for (std::size_t index = 0; index < fingerprint.bytes.size(); ++index) {
        fingerprint.bytes[index] = static_cast<std::uint8_t>((seed + index * 5U) % 256U);
    }
    return fingerprint;
}

void populate_index(biopic::SimilarityIndex& index, std::size_t count) {
    for (std::size_t entry = 0; entry < count; ++entry) {
        biopic::FingerprintRecord record;
        record.id = std::to_string(entry);
        record.fingerprint = make_fingerprint(static_cast<std::uint8_t>(entry % 255U));
        record.label = "sample";
        ASSERT_TRUE(index.add(record));
    }
}

} // namespace

TEST(FingerprintBucketTest, BandBucketsCoverAllBandsAndPrefix) {
    const biopic::Fingerprint fingerprint = make_fingerprint(19);
    const auto refs = biopic::band_bucket_refs(fingerprint);
    ASSERT_EQ(refs.size(), biopic::kFingerprintBandCount + 1U);
}

TEST(BucketedIndexTest, MatchesBruteForceNearestNeighbor) {
    biopic::BruteForceIndex brute_force;
    biopic::BucketedIndex bucketed;
    populate_index(brute_force, 256);
    populate_index(bucketed, 256);

    biopic::Fingerprint query = make_fingerprint(42);
    query.bytes[0] = static_cast<std::uint8_t>(query.bytes[0] + 1U);

    const biopic::HashMatchConfig config = biopic::kDefaultHashMatchConfig;
    const auto brute_result = brute_force.find_nearest(query, config);
    const auto bucketed_result = bucketed.find_nearest(query, config);

    EXPECT_EQ(brute_result.exact_match, bucketed_result.exact_match);
    EXPECT_NEAR(brute_result.distance, bucketed_result.distance, 1e-6);
    ASSERT_EQ(brute_result.record.has_value(), bucketed_result.record.has_value());
    if (brute_result.record.has_value()) {
        EXPECT_EQ(brute_result.record->id, bucketed_result.record->id);
    }
}

TEST(BucketedIndexTest, MatchesBruteForceSimilarQuery) {
    biopic::BruteForceIndex brute_force;
    biopic::BucketedIndex bucketed;
    populate_index(brute_force, 256);
    populate_index(bucketed, 256);

    biopic::Fingerprint query = make_fingerprint(42);
    query.bytes[0] = static_cast<std::uint8_t>(query.bytes[0] + 1U);

    biopic::HashMatchConfig config = biopic::kDefaultHashMatchConfig;
    config.threshold = 50.0;

    const auto brute_results = brute_force.query(query, config);
    const auto bucketed_results = bucketed.query(query, config);
    ASSERT_EQ(brute_results.size(), bucketed_results.size());
    for (std::size_t index = 0; index < brute_results.size(); ++index) {
        EXPECT_EQ(brute_results[index].matched.id, bucketed_results[index].matched.id);
        EXPECT_NEAR(brute_results[index].distance, bucketed_results[index].distance, 1e-6);
    }
}

TEST(BucketedIndexTest, CandidateCountStaysSmallAtTenThousandRecords) {
    biopic::BucketedIndex bucketed;
    populate_index(bucketed, 10'000U);

    biopic::Fingerprint query = make_fingerprint(42);
    query.bytes[0] = static_cast<std::uint8_t>(query.bytes[0] + 1U);

    const std::size_t candidate_count =
        bucketed.count_query_candidates(query, biopic::kDefaultHashMatchConfig, true);
    EXPECT_LT(candidate_count, 10'000U);
    EXPECT_LT(candidate_count, 2'000U);
    EXPECT_GT(candidate_count, 0U);
}
