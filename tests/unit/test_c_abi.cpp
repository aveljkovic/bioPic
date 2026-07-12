#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "biopic.h"

#include "test_images.hpp"

TEST(CAbiTest, HashRgbAndCompare) {
    const auto image = biopic::test_support::make_gradient_rgb(8, 8);
    BiopicEngine* engine = biopic_engine_create();
    ASSERT_NE(engine, nullptr);

    BiopicFingerprint fingerprint{};
    const BiopicStatus status =
        biopic_hash_rgb(engine, image.width, image.height, image.rgb.data(), image.rgb.size(),
                        &fingerprint);
    EXPECT_EQ(status, BIOPIC_OK);
    EXPECT_EQ(fingerprint.version, BIOPIC_HASH_VERSION_1);

    BiopicDistance distance{};
    EXPECT_EQ(biopic_compare_hashes(&fingerprint, &fingerprint, BIOPIC_DISTANCE_L1, &distance),
              BIOPIC_OK);
    EXPECT_DOUBLE_EQ(distance.value, 0.0);

    biopic_engine_destroy(engine);
}

TEST(CAbiTest, ReportsDecodeErrors) {
    BiopicEngine* engine = biopic_engine_create();
    ASSERT_NE(engine, nullptr);

    const std::uint8_t garbage[] = {0, 1, 2, 3};
    BiopicFingerprint fingerprint{};
    EXPECT_EQ(biopic_hash_image(engine, garbage, sizeof(garbage), &fingerprint),
              BIOPIC_ERR_DECODE);
    EXPECT_NE(std::string(biopic_last_error()), "");

    biopic_engine_destroy(engine);
}

TEST(CAbiTest, LastErrorPointerRemainsValidAfterReturn) {
    BiopicEngine* engine = biopic_engine_create();
    ASSERT_NE(engine, nullptr);

    const std::uint8_t garbage[] = {0, 1, 2, 3};
    BiopicFingerprint fingerprint{};
    EXPECT_EQ(biopic_hash_image(engine, garbage, sizeof(garbage), &fingerprint),
              BIOPIC_ERR_DECODE);

    const char* first_error = biopic_last_error();
    ASSERT_NE(first_error, nullptr);
    EXPECT_STREQ(first_error, "Image decode failed");

    const std::string copied(first_error);
    EXPECT_EQ(copied, "Image decode failed");
    EXPECT_STREQ(biopic_last_error(), copied.c_str());

    EXPECT_EQ(biopic_hash_image(nullptr, garbage, sizeof(garbage), &fingerprint),
              BIOPIC_ERR_INVALID_ARGUMENT);
    EXPECT_STREQ(biopic_last_error(), "Invalid argument");

    biopic_engine_destroy(engine);
}
