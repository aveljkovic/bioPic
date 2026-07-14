#include <filesystem>
#include <gtest/gtest.h>

#include "biopic/evaluation/dataset_manifest.hpp"
#include "biopic/evaluation/metrics.hpp"
#include "biopic/types.hpp"

TEST(EvaluationMetricsTest, PerfectBlockClassification) {
    biopic::evaluation::ConfusionMatrix matrix{};
    biopic::evaluation::record_prediction(matrix, biopic::ModerationDecision::Block,
                                          biopic::ModerationDecision::Block);
    biopic::evaluation::record_prediction(matrix, biopic::ModerationDecision::Allow,
                                          biopic::ModerationDecision::Allow);

    const biopic::evaluation::EvaluationReport report =
        biopic::evaluation::compute_report(matrix, 0);
    EXPECT_DOUBLE_EQ(report.accuracy, 1.0);
    EXPECT_DOUBLE_EQ(report.block.precision, 1.0);
    EXPECT_DOUBLE_EQ(report.block.recall, 1.0);
}

TEST(EvaluationManifestTest, ParsesDecisionLabels) {
    EXPECT_EQ(biopic::evaluation::parse_expected_decision("allow"),
              biopic::ModerationDecision::Allow);
    EXPECT_EQ(biopic::evaluation::parse_expected_decision("review"),
              biopic::ModerationDecision::Review);
    EXPECT_EQ(biopic::evaluation::parse_expected_decision("block"),
              biopic::ModerationDecision::Block);
}

TEST(EvaluationManifestTest, LoadsManifestCsv) {
    const auto samples = biopic::evaluation::load_manifest(
        std::filesystem::path(BIOPIC_TEST_FIXTURES_DIR) / "eval");
    ASSERT_GE(samples.size(), 1U);
    EXPECT_TRUE(std::filesystem::exists(samples.front().image_path));
}
