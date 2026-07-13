#include <gtest/gtest.h>

#include "biopic/database/fingerprint_store.hpp"
#include "biopic/index/fingerprint_normalization.hpp"
#include "biopic/index/hash_match_config.hpp"
#include "biopic/index/similarity_index.hpp"
#include "biopic/types.hpp"

namespace {

biopic::Fingerprint make_fingerprint(std::uint8_t seed) {
    biopic::Fingerprint fingerprint;
    for (std::size_t index = 0; index < fingerprint.bytes.size(); ++index) {
        fingerprint.bytes[index] = static_cast<std::uint8_t>((seed + index * 3U) % 256U);
    }
    return fingerprint;
}

void populate_records(std::vector<biopic::FingerprintRecord>& records, std::size_t count) {
    records.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        biopic::FingerprintRecord record;
        record.id = std::to_string(index);
        record.fingerprint = make_fingerprint(static_cast<std::uint8_t>(index % 255U));
        record.label = "sample";
        records.push_back(record);
    }
}

} // namespace

TEST(SimilarityIndexTest, BruteForceFindsExactMatch) {
    biopic::BruteForceIndex index;
    const biopic::Fingerprint fingerprint = make_fingerprint(12);

    biopic::FingerprintRecord record;
    record.fingerprint = fingerprint;
    record.label = "blocked";
    ASSERT_TRUE(index.add(record));

    const biopic::NearestMatchResult nearest = index.find_nearest(fingerprint, biopic::kDefaultHashMatchConfig);
    EXPECT_TRUE(nearest.exact_match);
    EXPECT_DOUBLE_EQ(nearest.distance, 0.0);
    ASSERT_TRUE(nearest.record.has_value());
    EXPECT_EQ(nearest.record->label, "blocked");
}

TEST(SimilarityIndexTest, BruteForceFindsNearestNonExactMatch) {
    biopic::BruteForceIndex index;
    biopic::Fingerprint reference = make_fingerprint(20);
    biopic::Fingerprint query = reference;
    query.bytes[4] = static_cast<std::uint8_t>(query.bytes[4] + 1U);

    biopic::FingerprintRecord record;
    record.fingerprint = reference;
    record.label = "reference";
    ASSERT_TRUE(index.add(record));

    const biopic::NearestMatchResult nearest = index.find_nearest(query, biopic::kDefaultHashMatchConfig);
    EXPECT_FALSE(nearest.exact_match);
    EXPECT_NEAR(nearest.distance, 1.0, 1e-6);
    ASSERT_TRUE(nearest.record.has_value());
    EXPECT_EQ(nearest.record->label, "reference");
}

TEST(SimilarityIndexTest, BruteForceQueryRespectsThresholdAndLimit) {
    biopic::BruteForceIndex index;
    biopic::Fingerprint query = make_fingerprint(30);
    query.bytes[0] = static_cast<std::uint8_t>(query.bytes[0] + 1U);

    for (std::uint8_t seed : {30U, 31U, 90U}) {
        biopic::FingerprintRecord record;
        record.fingerprint = make_fingerprint(seed);
        record.label = std::to_string(seed);
        ASSERT_TRUE(index.add(record));
    }

    biopic::HashMatchConfig config = biopic::kDefaultHashMatchConfig;
    config.threshold = 5.0;

    const auto results = index.query(query, config, 1);
    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results.front().matched.label, "30");
    EXPECT_LT(results.front().distance, config.threshold);
}

TEST(SimilarityIndexTest, MatchesLegacyAmongRecordsHelpers) {
    std::vector<biopic::FingerprintRecord> records;
    populate_records(records, 8);

    biopic::Fingerprint query = records[3].fingerprint;
    query.bytes[7] = static_cast<std::uint8_t>(query.bytes[7] + 2U);

    const auto legacy_nearest = biopic::find_nearest_among_records(records, query);
    const auto legacy_similar = biopic::find_similar_among_records(records, query, 10.0);

    biopic::BruteForceIndex index;
    for (const biopic::FingerprintRecord& record : records) {
        ASSERT_TRUE(index.add(record));
    }

    const auto indexed_nearest = index.find_nearest(query, biopic::kDefaultHashMatchConfig);
    biopic::HashMatchConfig config = biopic::kDefaultHashMatchConfig;
    config.threshold = 10.0;
    const auto indexed_similar = index.query(query, config);

    EXPECT_EQ(legacy_nearest.exact_match, indexed_nearest.exact_match);
    EXPECT_NEAR(legacy_nearest.distance, indexed_nearest.distance, 1e-6);
    ASSERT_EQ(legacy_nearest.record.has_value(), indexed_nearest.record.has_value());
    if (legacy_nearest.record.has_value()) {
        EXPECT_EQ(legacy_nearest.record->id, indexed_nearest.record->id);
    }

    ASSERT_EQ(legacy_similar.size(), indexed_similar.size());
    for (std::size_t index = 0; index < legacy_similar.size(); ++index) {
        EXPECT_EQ(legacy_similar[index].id, indexed_similar[index].matched.id);
    }
}

TEST(FingerprintNormalizationTest, ProducesUnitLengthVector) {
    const biopic::Fingerprint fingerprint = make_fingerprint(44);
    const biopic::NormalizedFingerprint normalized = biopic::normalize_fingerprint(fingerprint);

    double sum_of_squares = 0.0;
    for (double value : normalized) {
        sum_of_squares += value * value;
    }
    EXPECT_NEAR(sum_of_squares, 1.0, 1e-9);
}

TEST(FingerprintNormalizationTest, NormalizedDistanceIsZeroForIdenticalFingerprints) {
    const biopic::Fingerprint fingerprint = make_fingerprint(77);
    const biopic::HashMatchConfig config{.metric = biopic::DistanceMetric::L2,
                                         .threshold = 0.5,
                                         .use_normalized = true};
    EXPECT_DOUBLE_EQ(biopic::fingerprint_distance(fingerprint, fingerprint, config), 0.0);
}

TEST(HashMatchConfigTest, ResolvesCliAndEnvironmentOverrides) {
    const auto metric = biopic::parse_distance_metric("l1");
    ASSERT_TRUE(metric.has_value());
    EXPECT_EQ(*metric, biopic::DistanceMetric::L1);

    const biopic::HashMatchConfig config =
        biopic::resolve_hash_match_config(4.5, biopic::DistanceMetric::L2Squared);
    EXPECT_DOUBLE_EQ(config.threshold, 4.5);
    EXPECT_EQ(config.metric, biopic::DistanceMetric::L2Squared);
}
