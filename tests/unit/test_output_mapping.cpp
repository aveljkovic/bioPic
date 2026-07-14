#include <gtest/gtest.h>

#include "biopic/ai/output_mapping.hpp"

TEST(OutputMappingTest, MapsArgmaxLabelAndDetection) {
    biopic::OutputMapping mapping;
    mapping.labels = {"safe", "review", "blocked"};
    mapping.detection_label = "blocked";
    mapping.detection_threshold = 0.6;

    const auto result = mapping.map_scores({0.1f, 0.2f, 0.9f});
    EXPECT_EQ(result.label, "blocked");
    EXPECT_NEAR(result.confidence, 0.9, 1e-6);
    EXPECT_TRUE(result.detected);
}

TEST(OutputMappingTest, BelowThresholdIsNotDetected) {
    biopic::OutputMapping mapping;
    mapping.labels = {"safe", "review"};
    mapping.detection_label = "review";
    mapping.detection_threshold = 0.75;

    const auto result = mapping.map_scores({0.1f, 0.5f});
    EXPECT_EQ(result.label, "review");
    EXPECT_NEAR(result.confidence, 0.5, 1e-6);
    EXPECT_FALSE(result.detected);
}

TEST(OutputMappingTest, MismatchedOutputSizeReportsError) {
    biopic::OutputMapping mapping;
    mapping.labels = {"safe", "review"};

    const auto result = mapping.map_scores({0.2f});
    EXPECT_EQ(result.label, "output_shape_mismatch");
    EXPECT_FALSE(result.detected);
}

TEST(OutputMappingTest, AggregatesSpatialOutputsForManifestLabels) {
    biopic::OutputMapping mapping;
    mapping.labels = {"channel_r", "channel_g", "channel_b"};
    mapping.detection_label = "channel_r";
    mapping.detection_threshold = 1.0;

    biopic::InferenceOutput output;
    output.values.assign(48, 0.0f);
    for (std::size_t index = 0; index < 16; ++index) {
        output.values[index] = 0.04f;
    }
    for (std::size_t index = 16; index < 32; ++index) {
        output.values[index] = 0.08f;
    }
    for (std::size_t index = 32; index < 48; ++index) {
        output.values[index] = 0.12f;
    }

    const auto result = mapping.map(output);
    EXPECT_EQ(result.label, "channel_b");
    EXPECT_NEAR(result.confidence, 0.12, 1e-6);
    EXPECT_FALSE(result.detected);
}
