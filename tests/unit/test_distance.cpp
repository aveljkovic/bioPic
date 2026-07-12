#include <gtest/gtest.h>

#include <cmath>

#include "biopic/distance.hpp"
#include "biopic/fingerprint.hpp"

TEST(DistanceTest, IdenticalFingerprintsHaveZeroDistance) {
    biopic::Fingerprint fingerprint;
    fingerprint.bytes.fill(42);

    const auto l1 = biopic::compare_fingerprints(fingerprint, fingerprint, biopic::DistanceMetric::L1);
    const auto l2 = biopic::compare_fingerprints(fingerprint, fingerprint, biopic::DistanceMetric::L2);
    const auto l2s =
        biopic::compare_fingerprints(fingerprint, fingerprint, biopic::DistanceMetric::L2Squared);

    EXPECT_DOUBLE_EQ(l1.value, 0.0);
    EXPECT_DOUBLE_EQ(l2.value, 0.0);
    EXPECT_DOUBLE_EQ(l2s.value, 0.0);
}

TEST(DistanceTest, L1AndL2MatchManualComputation) {
    biopic::Fingerprint a;
    biopic::Fingerprint b;
    a.bytes = {0, 10, 20};
    b.bytes = {1, 8, 25};
    std::fill(a.bytes.begin() + 3, a.bytes.end(), 0);
    std::fill(b.bytes.begin() + 3, b.bytes.end(), 0);

    EXPECT_EQ(biopic::l1_distance(a.bytes, b.bytes), 1 + 2 + 5);
    EXPECT_EQ(biopic::l2_squared_distance(a.bytes, b.bytes), 1 + 4 + 25);
    EXPECT_NEAR(biopic::l2_distance(a.bytes, b.bytes), std::sqrt(30.0), 1e-9);
}
