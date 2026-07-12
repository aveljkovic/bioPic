#include <gtest/gtest.h>

#include "biopic/hasher.hpp"
#include "biopic/image.hpp"

#include "test_images.hpp"

TEST(HasherTest, EmptyImageProducesZeroFingerprint) {
    biopic::ImageView view;
    biopic::Hasher hasher;
    const biopic::Fingerprint fingerprint = hasher.compute(view);
    EXPECT_TRUE(fingerprint.is_zero());
}

TEST(HasherTest, OnePixelUniformImageProducesZeroFingerprint) {
    const auto image = biopic::test_support::make_uniform_rgb(1, 1, 255, 128, 64);
    biopic::ImageView view(image.width, image.height, image.rgb);
    biopic::Hasher hasher;
    const auto fingerprint = hasher.compute(view);
    EXPECT_TRUE(fingerprint.is_zero());
}

TEST(HasherTest, ExtremeAspectRatioProducesFingerprint) {
    const auto image = biopic::test_support::make_gradient_rgb(640, 32);
    biopic::ImageView view(image.width, image.height, image.rgb);
    biopic::Hasher hasher;
    const auto fingerprint = hasher.compute(view);
    EXPECT_EQ(fingerprint.version, 1U);
    EXPECT_FALSE(fingerprint.is_zero());
}
