#include <gtest/gtest.h>

#include "biopic/ai/classifier.hpp"
#include "biopic/image.hpp"

#include "test_images.hpp"

TEST(ClassifierTest, ClassificationResultNotDetectedFactory) {
    const auto result = biopic::ClassificationResult::not_detected("safe");
    EXPECT_EQ(result.label, "safe");
    EXPECT_DOUBLE_EQ(result.confidence, 0.0);
    EXPECT_FALSE(result.detected);
}

TEST(ClassifierTest, DummyClassifierReturnsSafeForGradientImage) {
    const auto image = biopic::test_support::make_gradient_rgb(64, 48);
    biopic::ImageView view(image.width, image.height, image.rgb);

    biopic::DummyClassifier classifier(biopic::ClassifierKind::Nudity);
    EXPECT_EQ(classifier.kind(), biopic::ClassifierKind::Nudity);
    EXPECT_TRUE(classifier.model().is_loaded());

    const auto result = classifier.classify(view);
    EXPECT_EQ(result.label, "safe");
    EXPECT_DOUBLE_EQ(result.confidence, 0.0);
    EXPECT_FALSE(result.detected);
}

TEST(ClassifierTest, DummyClassifierSupportsAllPlannedKinds) {
    const auto image = biopic::test_support::make_uniform_rgb(16, 16, 128, 64, 32);
    biopic::ImageView view(image.width, image.height, image.rgb);

    const biopic::ClassifierKind kinds[] = {
        biopic::ClassifierKind::Nudity,    biopic::ClassifierKind::Violence,
        biopic::ClassifierKind::Weapons,   biopic::ClassifierKind::Drugs,
        biopic::ClassifierKind::ChildSafety,
    };

    for (const auto kind : kinds) {
        biopic::DummyClassifier classifier(kind);
        EXPECT_EQ(classifier.kind(), kind);
        EXPECT_FALSE(classifier.classify(view).detected);
    }
}

TEST(ClassifierTest, DummyClassifierRejectsEmptyImage) {
    biopic::ImageView view;
    biopic::DummyClassifier classifier;
    const auto result = classifier.classify(view);
    EXPECT_EQ(result.label, "invalid_input");
    EXPECT_FALSE(result.detected);
}
