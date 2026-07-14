#include <gtest/gtest.h>

#include <cmath>

#include "biopic/ai/preprocessing.hpp"
#include "biopic/image.hpp"

#include "test_images.hpp"

TEST(PreprocessingTest, PrepareProducesChannelFirstTensor) {
    const auto image = biopic::test_support::make_uniform_rgb(32, 24, 200, 100, 50);
    biopic::ImageView view(image.width, image.height, image.rgb);

    biopic::Preprocessor preprocessor(biopic::ModelInputRequirements{16, 12, 3});
    const auto tensor = preprocessor.prepare(view);
    ASSERT_TRUE(tensor.has_value());
    EXPECT_EQ(tensor->width, 16);
    EXPECT_EQ(tensor->height, 12);
    EXPECT_EQ(tensor->channels, 3);
    EXPECT_EQ(tensor->data.size(), static_cast<std::size_t>(16 * 12 * 3));
}

TEST(PreprocessingTest, NormalizationUsesConfiguredScale) {
    biopic::ModelInputRequirements requirements;
    requirements.width = 1;
    requirements.height = 1;
    requirements.channels = 3;
    requirements.scale = 1.0 / 255.0;
    requirements.mean = {0.0f, 0.0f, 0.0f};
    requirements.stddev = {1.0f, 1.0f, 1.0f};

    const auto image = biopic::test_support::make_uniform_rgb(1, 1, 255, 0, 128);
    biopic::ImageView view(image.width, image.height, image.rgb);

    biopic::Preprocessor preprocessor(requirements);
    const auto tensor = preprocessor.prepare(view);
    ASSERT_TRUE(tensor.has_value());
    EXPECT_NEAR(tensor->data[0], 1.0f, 1e-6f);
    EXPECT_NEAR(tensor->data[1], 0.0f, 1e-6f);
    EXPECT_NEAR(tensor->data[2], 128.0f / 255.0f, 1e-6f);
}

TEST(PreprocessingTest, EmptyImageReturnsNullopt) {
    biopic::Preprocessor preprocessor(biopic::ModelInputRequirements{});
    biopic::ImageView view;
    EXPECT_FALSE(preprocessor.prepare(view).has_value());
}
