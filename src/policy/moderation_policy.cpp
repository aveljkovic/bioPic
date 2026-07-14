#include "biopic/policy/moderation_policy.hpp"

#include "pipeline/scanner.hpp"

namespace biopic {

namespace {

PolicyEvaluation decision_from_classifier(const ScanResult& result, const PolicyConfig& config) {
    if (!result.classification.has_value()) {
        return {};
    }

    const ClassificationResult& classification = *result.classification;
    if (!classification.detected) {
        return {};
    }

    if (classification.confidence >= config.classifier_block_threshold) {
        return PolicyEvaluation{ModerationDecision::Block, ModerationReason::ModelClassification};
    }

    if (classification.confidence >= config.classifier_review_threshold) {
        return PolicyEvaluation{ModerationDecision::Review, ModerationReason::ModelClassification};
    }

    return {};
}

PolicyEvaluation decision_from_database(const ScanResult& result, const PolicyConfig& config) {
    switch (result.match_status) {
    case MatchStatus::ExactMatch:
        return PolicyEvaluation{config.exact_match_decision, ModerationReason::KnownHashMatch};
    case MatchStatus::SimilarMatch:
        return PolicyEvaluation{config.similar_match_decision, ModerationReason::SimilarHashMatch};
    case MatchStatus::NoMatch:
        break;
    }
    return {};
}

[[nodiscard]] int decision_severity(ModerationDecision decision) {
    switch (decision) {
    case ModerationDecision::Block:
        return 3;
    case ModerationDecision::Review:
        return 2;
    case ModerationDecision::Allow:
        return 1;
    }
    return 0;
}

PolicyEvaluation merge_evaluations(const PolicyEvaluation& left, const PolicyEvaluation& right) {
    if (decision_severity(right.decision) > decision_severity(left.decision)) {
        return right;
    }
    if (decision_severity(right.decision) == decision_severity(left.decision) &&
        right.reason != ModerationReason::None) {
        return right;
    }
    return left;
}

} // namespace

ModerationPolicy::ModerationPolicy(PolicyConfig config) : config_(config) {}

PolicyEvaluation ModerationPolicy::evaluate(const ScanResult& result) const {
    PolicyEvaluation evaluation{config_.default_decision, ModerationReason::None};

    evaluation = merge_evaluations(evaluation, decision_from_database(result, config_));
    evaluation = merge_evaluations(evaluation, decision_from_classifier(result, config_));

    return evaluation;
}

PolicyEvaluation evaluate_policy(const ScanResult& result, const PolicyConfig& config) {
    return ModerationPolicy(config).evaluate(result);
}

ModerationDecision scan_decision(const ScanResult& result) {
    return evaluate_policy(result).decision;
}

ModerationDecision scan_decision(const ScanResult& result, const PolicyConfig& config) {
    return evaluate_policy(result, config).decision;
}

const char* moderation_reason_label(ModerationReason reason) {
    switch (reason) {
    case ModerationReason::None:
        return "none";
    case ModerationReason::KnownHashMatch:
        return "exact fingerprint match";
    case ModerationReason::SimilarHashMatch:
        return "similar fingerprint match";
    case ModerationReason::ModelClassification:
        return "classifier score";
    case ModerationReason::InvalidInput:
        return "invalid input";
    case ModerationReason::ProcessingError:
        return "processing error";
    }
    return "unknown";
}

} // namespace biopic
