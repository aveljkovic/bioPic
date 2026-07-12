#include <gtest/gtest.h>

#include "biopic/distance.hpp"
#include "biopic/fingerprint.hpp"
#include "biopic/hasher.hpp"
#include "biopic/image.hpp"

#include "test_images.hpp"

TEST(DeterminismTest, RepeatedHashingIsStable) {
    const auto image = biopic::test_support::make_gradient_rgb(128, 96);
    biopic::ImageView view(image.width, image.height, image.rgb);
    biopic::Hasher hasher;

    const auto baseline = hasher.compute(view);
    for (int i = 0; i < 32; ++i) {
        const auto fingerprint = hasher.compute(view);
        EXPECT_EQ(fingerprint.bytes, baseline.bytes);
    }
}

TEST(DeterminismTest, SerializedRoundTripPreservesDistance) {
    const auto left = biopic::test_support::make_checkerboard_rgb(48, 48);
    const auto right = biopic::test_support::make_checkerboard_rgb(48, 48, 6);
    biopic::ImageView left_view(left.width, left.height, left.rgb);
    biopic::ImageView right_view(right.width, right.height, right.rgb);
    biopic::Hasher hasher;

    const auto left_fp = hasher.compute(left_view);
    const auto right_fp = hasher.compute(right_view);

    const std::string encoded =
        biopic::encode_fingerprint(left_fp, biopic::FingerprintEncoding::Hex);
    const auto decoded = biopic::decode_fingerprint(encoded, biopic::FingerprintEncoding::Hex);
    ASSERT_TRUE(decoded.has_value());

    const auto distance =
        biopic::compare_fingerprints(*decoded, right_fp, biopic::DistanceMetric::L1);
    const auto baseline =
        biopic::compare_fingerprints(left_fp, right_fp, biopic::DistanceMetric::L1);
    EXPECT_DOUBLE_EQ(distance.value, baseline.value);
}
