#include <gtest/gtest.h>

#include <unordered_set>

#include "biopic/index/bucketed_index.hpp"
#include "biopic/index/fingerprint_bucket.hpp"
#include "biopic/index/similarity_index.hpp"

namespace {

biopic::Fingerprint make_fingerprint(std::size_t seed) {
    biopic::Fingerprint fingerprint;
    for (std::size_t index = 0; index < fingerprint.bytes.size(); ++index) {
        fingerprint.bytes[index] =
            static_cast<std::uint8_t>((seed + index * 5U + (seed >> 4)) % 256U);
    }
    return fingerprint;
}

void populate_index(biopic::SimilarityIndex& index, std::size_t count) {
    for (std::size_t entry = 0; entry < count; ++entry) {
        biopic::FingerprintRecord record;
        record.id = std::to_string(entry);
        record.fingerprint = make_fingerprint(entry);
        record.label = "sample";
        ASSERT_TRUE(index.add(record));
    }
}

double measure_similar_query_recall(biopic::BruteForceIndex& brute_force,
                                    biopic::BucketedIndex& bucketed,
                                    const biopic::Fingerprint& query,
                                    const biopic::HashMatchConfig& config) {
    const auto brute_results = brute_force.query(query, config);
    const auto bucketed_results = bucketed.query(query, config);

    std::unordered_set<std::string> bucketed_ids;
    bucketed_ids.reserve(bucketed_results.size());
    for (const biopic::SimilarityQueryResult& result : bucketed_results) {
        bucketed_ids.insert(result.matched.id);
    }

    if (brute_results.empty()) {
        return 1.0;
    }

    std::size_t recovered = 0;
    for (const biopic::SimilarityQueryResult& result : brute_results) {
        if (bucketed_ids.count(result.matched.id) > 0) {
            ++recovered;
        }
    }
    return static_cast<double>(recovered) / static_cast<double>(brute_results.size());
}

} // namespace

TEST(FingerprintBucketTest, BandBucketsCoverAllLshSlots) {
    const biopic::Fingerprint fingerprint = make_fingerprint(19);
    const auto refs = biopic::band_bucket_refs(fingerprint);
    ASSERT_EQ(refs.size(), biopic::kFingerprintBandSlotCount);
}

TEST(FingerprintBucketTest, SimilarQueryUsesNeighborBuckets) {
    const biopic::Fingerprint fingerprint = make_fingerprint(19);
    biopic::HashMatchConfig config = biopic::kDefaultHashMatchConfig;
    config.threshold = 50.0;

    const auto exact_refs = biopic::band_bucket_refs(fingerprint);
    const auto candidate_refs = biopic::candidate_bucket_refs(fingerprint, config, false);
    EXPECT_GT(candidate_refs.size(), exact_refs.size());
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

TEST(BucketedIndexTest, SimilarQueryRecallAtLeast99PercentOnBenchmarkDataset) {
    biopic::BruteForceIndex brute_force;
    biopic::BucketedIndex bucketed;
    constexpr std::size_t kRecordCount = 1'000U;
    populate_index(brute_force, kRecordCount);
    populate_index(bucketed, kRecordCount);

    biopic::HashMatchConfig config = biopic::kDefaultHashMatchConfig;
    config.threshold = 50.0;

    double total_recall = 0.0;
    constexpr std::size_t kQueryCount = 32U;
    for (std::size_t query_index = 0; query_index < kQueryCount; ++query_index) {
        biopic::Fingerprint query = make_fingerprint(query_index + 3U);
        query.bytes[query_index % biopic::kFingerprintComponentCount] =
            static_cast<std::uint8_t>(query.bytes[query_index % biopic::kFingerprintComponentCount] +
                                      1U);
        total_recall += measure_similar_query_recall(brute_force, bucketed, query, config);
    }

    const double average_recall = total_recall / static_cast<double>(kQueryCount);
    EXPECT_GE(average_recall, 0.99);
}

TEST(BucketedIndexTest, NearestCandidateCountBelowOnePercentAtOneHundredThousandRecords) {
    biopic::BucketedIndex bucketed;
    populate_index(bucketed, 100'000U);

    biopic::Fingerprint query = make_fingerprint(42);
    query.bytes[0] = static_cast<std::uint8_t>(query.bytes[0] + 1U);

    const std::size_t candidate_count =
        bucketed.count_query_candidates(query, biopic::kDefaultHashMatchConfig, true);
    EXPECT_LT(candidate_count, 100'000U);
    EXPECT_LT(candidate_count, 1'000U);
    EXPECT_GT(candidate_count, 0U);
}

TEST(BucketedIndexTest, NearestCandidateCountStaysSmallAtTenThousandRecords) {
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
