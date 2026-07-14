#include <gtest/gtest.h>

#include "biopic/policy/moderation_policy.hpp"
#include "pipeline/scanner.hpp"

namespace {

biopic::ScanResult make_result() { return biopic::ScanResult{}; }

} // namespace

TEST(ModerationPolicyTest, ExactFingerprintMatchBlocks) {
    biopic::ScanResult result = make_result();
    result.match_status = biopic::MatchStatus::ExactMatch;

    const biopic::PolicyEvaluation evaluation = biopic::evaluate_policy(result);
    EXPECT_EQ(evaluation.decision, biopic::ModerationDecision::Block);
    EXPECT_EQ(evaluation.reason, biopic::ModerationReason::KnownHashMatch);
}

TEST(ModerationPolicyTest, SimilarFingerprintMatchReviews) {
    biopic::ScanResult result = make_result();
    result.match_status = biopic::MatchStatus::SimilarMatch;

    const biopic::PolicyEvaluation evaluation = biopic::evaluate_policy(result);
    EXPECT_EQ(evaluation.decision, biopic::ModerationDecision::Review);
    EXPECT_EQ(evaluation.reason, biopic::ModerationReason::SimilarHashMatch);
}

TEST(ModerationPolicyTest, HighConfidenceClassifierBlocks) {
    biopic::ScanResult result = make_result();
    result.classification = biopic::ClassificationResult{"explicit", 0.98, true};

    const biopic::PolicyEvaluation evaluation = biopic::evaluate_policy(result);
    EXPECT_EQ(evaluation.decision, biopic::ModerationDecision::Block);
    EXPECT_EQ(evaluation.reason, biopic::ModerationReason::ModelClassification);
}

TEST(ModerationPolicyTest, MediumConfidenceClassifierReviews) {
    biopic::ScanResult result = make_result();
    result.classification = biopic::ClassificationResult{"explicit", 0.85, true};

    const biopic::PolicyEvaluation evaluation = biopic::evaluate_policy(result);
    EXPECT_EQ(evaluation.decision, biopic::ModerationDecision::Review);
    EXPECT_EQ(evaluation.reason, biopic::ModerationReason::ModelClassification);
}

TEST(ModerationPolicyTest, LowConfidenceClassifierAllows) {
    biopic::ScanResult result = make_result();
    result.classification = biopic::ClassificationResult{"explicit", 0.55, true};

    const biopic::PolicyEvaluation evaluation = biopic::evaluate_policy(result);
    EXPECT_EQ(evaluation.decision, biopic::ModerationDecision::Allow);
    EXPECT_EQ(evaluation.reason, biopic::ModerationReason::None);
}

TEST(ModerationPolicyTest, NoSignalsAllow) {
    const biopic::ScanResult result = make_result();
    const biopic::PolicyEvaluation evaluation = biopic::evaluate_policy(result);
    EXPECT_EQ(evaluation.decision, biopic::ModerationDecision::Allow);
    EXPECT_EQ(evaluation.reason, biopic::ModerationReason::None);
}

TEST(ModerationPolicyTest, ClassifierBlockOverridesSimilarReview) {
    biopic::ScanResult result = make_result();
    result.match_status = biopic::MatchStatus::SimilarMatch;
    result.classification = biopic::ClassificationResult{"explicit", 0.99, true};

    const biopic::PolicyEvaluation evaluation = biopic::evaluate_policy(result);
    EXPECT_EQ(evaluation.decision, biopic::ModerationDecision::Block);
    EXPECT_EQ(evaluation.reason, biopic::ModerationReason::ModelClassification);
}

TEST(ModerationPolicyTest, CustomPolicyConfig) {
    biopic::PolicyConfig config;
    config.similar_match_decision = biopic::ModerationDecision::Allow;

    biopic::ScanResult result = make_result();
    result.match_status = biopic::MatchStatus::SimilarMatch;

    const biopic::PolicyEvaluation evaluation = biopic::evaluate_policy(result, config);
    EXPECT_EQ(evaluation.decision, biopic::ModerationDecision::Allow);
}

TEST(ModerationPolicyTest, ScanDecisionWrapperMatchesPolicy) {
    biopic::ScanResult result = make_result();
    result.match_status = biopic::MatchStatus::ExactMatch;
    EXPECT_EQ(biopic::scan_decision(result), biopic::evaluate_policy(result).decision);
}
