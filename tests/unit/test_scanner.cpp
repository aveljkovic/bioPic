#include <gtest/gtest.h>

#include "biopic/ai/classifier.hpp"
#include "biopic/ai/classifier_config.hpp"
#include "biopic/database/fingerprint_store.hpp"
#include "biopic/hasher.hpp"
#include "biopic/types.hpp"
#include "pipeline/scanner.hpp"

#include "test_images.hpp"

namespace {

std::filesystem::path fixture_path(const char* name) {
    return std::filesystem::path(BIOPIC_TEST_FIXTURES_DIR) / name;
}

} // namespace

TEST(ScannerTest, ScanWithoutClassifierConfiguration) {
    const auto image = biopic::test_support::make_uniform_rgb(32, 24, 120, 80, 40);
    biopic::ImageView view(image.width, image.height, image.rgb);

    const biopic::ScanResult result = biopic::scan(view);

    const biopic::Hasher hasher;
    EXPECT_EQ(result.fingerprint.bytes, hasher.compute(view).bytes);
    EXPECT_EQ(result.classifier_availability, biopic::ClassifierAvailability::Skipped);
    EXPECT_EQ(result.classifier_status, "not configured");
    EXPECT_FALSE(result.classification.has_value());
    EXPECT_EQ(biopic::scan_decision(result), biopic::ModerationDecision::Allow);
}

TEST(ScannerTest, ScanWithInvalidClassifierConfigurationContinuesWithoutClassification) {
    const auto image = biopic::test_support::make_uniform_rgb(16, 16, 64, 32, 16);
    biopic::ImageView view(image.width, image.height, image.rgb);

    const std::filesystem::path missing_config =
        std::filesystem::temp_directory_path() / "biopic_missing_scanner_config.conf";
    const biopic::ScanResult result = biopic::scan(view, missing_config);

    const biopic::Hasher hasher;
    EXPECT_EQ(result.fingerprint.bytes, hasher.compute(view).bytes);
    EXPECT_EQ(result.classifier_availability, biopic::ClassifierAvailability::Unavailable);
    EXPECT_EQ(result.classifier_status, "configuration invalid");
    EXPECT_FALSE(result.classification.has_value());
    EXPECT_EQ(biopic::scan_decision(result), biopic::ModerationDecision::Allow);
}

TEST(ScannerTest, ScanWithClassifierAvailable) {
    const auto config_path = fixture_path("classifier.conf");
    const auto image = biopic::test_support::make_uniform_rgb(4, 4, 10, 20, 30);
    biopic::ImageView view(image.width, image.height, image.rgb);

    const biopic::ScanResult result = biopic::scan(view, config_path);

    const biopic::Hasher hasher;
    EXPECT_EQ(result.fingerprint.bytes, hasher.compute(view).bytes);
    EXPECT_EQ(result.classifier_availability, biopic::ClassifierAvailability::Available);
    EXPECT_EQ(result.classifier_status, "available");
    ASSERT_TRUE(result.classification.has_value());
    EXPECT_EQ(result.classification->label, "channel_b");
    EXPECT_FALSE(result.classification->detected);
    EXPECT_EQ(biopic::scan_decision(result), biopic::ModerationDecision::Allow);
}

TEST(ScannerTest, ScanWithInjectedClassifierCombinesFingerprintAndClassification) {
    const auto image = biopic::test_support::make_uniform_rgb(32, 24, 120, 80, 40);
    biopic::ImageView view(image.width, image.height, image.rgb);

    biopic::DummyClassifier classifier;
    const biopic::ScanResult result = biopic::scan(view, classifier, nullptr);

    const biopic::Hasher hasher;
    EXPECT_EQ(result.fingerprint.bytes, hasher.compute(view).bytes);
    ASSERT_TRUE(result.classification.has_value());
    EXPECT_EQ(result.classification->label, "safe");
    EXPECT_EQ(result.classifier_availability, biopic::ClassifierAvailability::Available);
}

TEST(ScannerTest, ScanDecisionBlockWhenDetected) {
    biopic::ScanResult result;
    result.classification = biopic::ClassificationResult{"explicit", 0.95, true};
    EXPECT_EQ(biopic::scan_decision(result), biopic::ModerationDecision::Block);
    EXPECT_STREQ(biopic::scan_decision_label(biopic::scan_decision(result)), "BLOCK");
}

TEST(ScannerTest, ScanFileDecodesOnceAndReturnsFingerprint) {
    const std::filesystem::path image_path = fixture_path("cli_classify.png");
    ASSERT_TRUE(std::filesystem::exists(image_path));

    const auto result = biopic::scan_file(image_path);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->classifier_availability, biopic::ClassifierAvailability::Skipped);
    EXPECT_FALSE(result->classification.has_value());
    EXPECT_EQ(result->fingerprint.version, biopic::kHashAlgorithmVersion);
    EXPECT_EQ(result->match_status, biopic::MatchStatus::NoMatch);
}

TEST(ScannerTest, ExactDatabaseMatchBlocksWithoutClassifier) {
    const auto image = biopic::test_support::make_uniform_rgb(16, 16, 90, 45, 20);
    biopic::ImageView view(image.width, image.height, image.rgb);

    biopic::Hasher hasher;
    biopic::FingerprintStore store;
    biopic::FingerprintRecord record;
    record.fingerprint = hasher.compute(view);
    record.label = "known_bad";
    ASSERT_TRUE(store.add(record));

    const biopic::ScanResult result = biopic::scan(view, std::nullopt, &store);
    EXPECT_EQ(result.match_status, biopic::MatchStatus::ExactMatch);
    ASSERT_TRUE(result.matched_record.has_value());
    EXPECT_EQ(result.matched_record->label, "known_bad");
    EXPECT_EQ(result.classifier_status, "skipped (database match)");
    EXPECT_FALSE(result.classification.has_value());
    EXPECT_EQ(biopic::scan_decision(result), biopic::ModerationDecision::Block);
}
