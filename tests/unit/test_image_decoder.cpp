#include <gtest/gtest.h>

#include <vector>

#include "biopic/image.hpp"

#include "test_images.hpp"

TEST(ImageDecoderTest, RejectsEmptyInput) {
    biopic::ImageDecoder decoder;
    const std::vector<std::uint8_t> empty;
    EXPECT_FALSE(decoder.decode(empty).has_value());
}

TEST(ImageDecoderTest, DecodesPngAndJpeg) {
    const auto image = biopic::test_support::make_checkerboard_rgb(64, 64);
    const auto png = biopic::test_support::encode_png(image);
    const auto jpeg = biopic::test_support::encode_jpeg(image);

    biopic::ImageDecoder decoder;
    const auto decoded_png = decoder.decode(png);
    const auto decoded_jpeg = decoder.decode(jpeg);
    ASSERT_TRUE(decoded_png.has_value());
    ASSERT_TRUE(decoded_jpeg.has_value());
    EXPECT_EQ(decoded_png->width, 64);
    EXPECT_EQ(decoded_png->height, 64);
    EXPECT_EQ(decoded_jpeg->width, 64);
    EXPECT_EQ(decoded_jpeg->height, 64);
    EXPECT_FALSE(decoded_png->sha256_hex.empty());
}

TEST(ImageDecoderTest, EnforcesEncodedByteLimit) {
    biopic::DecodeLimits limits;
    limits.max_encoded_bytes = 16;
    biopic::ImageDecoder decoder(limits);
    const auto image = biopic::test_support::make_gradient_rgb(128, 128);
    const auto png = biopic::test_support::encode_png(image);
    EXPECT_FALSE(decoder.decode(png).has_value());
}
