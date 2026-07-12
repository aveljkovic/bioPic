#include <gtest/gtest.h>

#include "biopic/hasher.hpp"
#include "biopic/image.hpp"

#include "test_images.hpp"

TEST(UniformColorTest, EveryUniformRgbImageProducesZeroFingerprint) {
    const struct {
        std::uint8_t r;
        std::uint8_t g;
        std::uint8_t b;
    } colors[] = {
        {0, 0, 0},
        {255, 255, 255},
        {255, 0, 0},
        {0, 255, 0},
        {0, 0, 255},
        {128, 128, 128},
        {64, 128, 192},
        {10, 20, 30},
    };

    biopic::Hasher hasher;
    for (const auto& color : colors) {
        const auto image = biopic::test_support::make_uniform_rgb(32, 32, color.r, color.g, color.b);
        biopic::ImageView view(image.width, image.height, image.rgb);
        const auto fingerprint = hasher.compute(view);
        EXPECT_TRUE(fingerprint.is_zero()) << "uniform color rgb(" << static_cast<int>(color.r)
                                           << ',' << static_cast<int>(color.g) << ','
                                           << static_cast<int>(color.b) << ')';
    }
}
