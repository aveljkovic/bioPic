#include <gtest/gtest.h>

#include <chrono>

#include "biopic/index/similarity_index.hpp"
#include "biopic/types.hpp"

namespace {

biopic::Fingerprint make_fingerprint(std::uint8_t seed) {
    biopic::Fingerprint fingerprint;
    for (std::size_t index = 0; index < fingerprint.bytes.size(); ++index) {
        fingerprint.bytes[index] = static_cast<std::uint8_t>((seed + index * 11U) % 256U);
    }
    return fingerprint;
}

void populate_index(biopic::BruteForceIndex& index, std::size_t count) {
    for (std::size_t entry = 0; entry < count; ++entry) {
        biopic::FingerprintRecord record;
        record.id = std::to_string(entry);
        record.fingerprint = make_fingerprint(static_cast<std::uint8_t>(entry % 255U));
        record.label = "sample";
        ASSERT_TRUE(index.add(record));
    }
}

} // namespace

TEST(SimilarityIndexPerformanceTest, BruteForceNearestSearchCompletesForTenThousandRecords) {
    biopic::BruteForceIndex index;
    populate_index(index, 10'000U);

    biopic::Fingerprint query = make_fingerprint(42);
    query.bytes[2] = static_cast<std::uint8_t>(query.bytes[2] + 1U);

    const auto start = std::chrono::steady_clock::now();
    const biopic::NearestMatchResult nearest = index.find_nearest(query, biopic::kDefaultHashMatchConfig);
    const auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(nearest.exact_match);
    EXPECT_LT(nearest.distance, biopic::kDefaultHashMatchConfig.threshold);
    EXPECT_LT(elapsed, std::chrono::milliseconds(500));
}

TEST(SimilarityIndexPerformanceTest, BruteForceSimilarQueryCompletesForTenThousandRecords) {
    biopic::BruteForceIndex index;
    populate_index(index, 10'000U);

    biopic::Fingerprint query = make_fingerprint(42);
    query.bytes[2] = static_cast<std::uint8_t>(query.bytes[2] + 1U);

    biopic::HashMatchConfig config = biopic::kDefaultHashMatchConfig;
    config.threshold = 50.0;

    const auto start = std::chrono::steady_clock::now();
    const auto results = index.query(query, config);
    const auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_FALSE(results.empty());
    EXPECT_LT(elapsed, std::chrono::milliseconds(500));
}
