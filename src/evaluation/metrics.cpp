#include "biopic/evaluation/metrics.hpp"

#include "biopic/types.hpp"

namespace biopic::evaluation {

namespace {

ClassMetrics metrics_for_class(const ConfusionMatrix& matrix, ModerationDecision decision) {
    ClassMetrics metrics;
    std::size_t true_positive = 0;
    std::size_t false_positive = 0;
    std::size_t false_negative = 0;

    switch (decision) {
    case ModerationDecision::Allow:
        metrics.support = matrix.allow_allow + matrix.review_allow + matrix.block_allow;
        true_positive = matrix.allow_allow;
        false_positive = matrix.allow_review + matrix.allow_block;
        false_negative = matrix.review_allow + matrix.block_allow;
        break;
    case ModerationDecision::Review:
        metrics.support = matrix.allow_review + matrix.review_review + matrix.block_review;
        true_positive = matrix.review_review;
        false_positive = matrix.review_allow + matrix.review_block;
        false_negative = matrix.allow_review + matrix.block_review;
        break;
    case ModerationDecision::Block:
        metrics.support = matrix.allow_block + matrix.review_block + matrix.block_block;
        true_positive = matrix.block_block;
        false_positive = matrix.block_allow + matrix.block_review;
        false_negative = matrix.allow_block + matrix.review_block;
        break;
    }

    if (true_positive + false_positive > 0) {
        metrics.precision =
            static_cast<double>(true_positive) / static_cast<double>(true_positive + false_positive);
    }
    if (true_positive + false_negative > 0) {
        metrics.recall =
            static_cast<double>(true_positive) / static_cast<double>(true_positive + false_negative);
    }
    if (metrics.precision + metrics.recall > 0.0) {
        metrics.f1 = 2.0 * metrics.precision * metrics.recall / (metrics.precision + metrics.recall);
    }
    return metrics;
}

} // namespace

void record_prediction(ConfusionMatrix& matrix, ModerationDecision expected,
                       ModerationDecision predicted) {
    if (expected == ModerationDecision::Allow && predicted == ModerationDecision::Allow) {
        ++matrix.allow_allow;
    } else if (expected == ModerationDecision::Allow && predicted == ModerationDecision::Review) {
        ++matrix.allow_review;
    } else if (expected == ModerationDecision::Allow && predicted == ModerationDecision::Block) {
        ++matrix.allow_block;
    } else if (expected == ModerationDecision::Review && predicted == ModerationDecision::Allow) {
        ++matrix.review_allow;
    } else if (expected == ModerationDecision::Review && predicted == ModerationDecision::Review) {
        ++matrix.review_review;
    } else if (expected == ModerationDecision::Review && predicted == ModerationDecision::Block) {
        ++matrix.review_block;
    } else if (expected == ModerationDecision::Block && predicted == ModerationDecision::Allow) {
        ++matrix.block_allow;
    } else if (expected == ModerationDecision::Block && predicted == ModerationDecision::Review) {
        ++matrix.block_review;
    } else if (expected == ModerationDecision::Block && predicted == ModerationDecision::Block) {
        ++matrix.block_block;
    }
}

EvaluationReport compute_report(const ConfusionMatrix& matrix, std::size_t skipped) {
    EvaluationReport report;
    report.confusion = matrix;
    report.skipped = skipped;
    report.evaluated = matrix.allow_allow + matrix.allow_review + matrix.allow_block +
                       matrix.review_allow + matrix.review_review + matrix.review_block +
                       matrix.block_allow + matrix.block_review + matrix.block_block;
    report.total = report.evaluated + skipped;

    if (report.evaluated == 0) {
        return report;
    }

    const std::size_t correct = matrix.allow_allow + matrix.review_review + matrix.block_block;
    report.accuracy = static_cast<double>(correct) / static_cast<double>(report.evaluated);

    report.allow = metrics_for_class(matrix, ModerationDecision::Allow);
    report.review = metrics_for_class(matrix, ModerationDecision::Review);
    report.block = metrics_for_class(matrix, ModerationDecision::Block);

    const std::size_t flagged_expected =
        matrix.allow_review + matrix.allow_block + matrix.review_allow + matrix.review_review +
        matrix.review_block + matrix.block_allow + matrix.block_review + matrix.block_block;
    const std::size_t flagged_predicted =
        matrix.allow_review + matrix.allow_block + matrix.review_review + matrix.review_block +
        matrix.block_review + matrix.block_block;
    const std::size_t flagged_true_positive =
        matrix.review_review + matrix.review_block + matrix.block_review + matrix.block_block;

    if (flagged_predicted > 0) {
        report.flagged_precision =
            static_cast<double>(flagged_true_positive) / static_cast<double>(flagged_predicted);
    }
    if (flagged_expected > 0) {
        report.flagged_recall =
            static_cast<double>(flagged_true_positive) / static_cast<double>(flagged_expected);
    }
    if (report.flagged_precision + report.flagged_recall > 0.0) {
        report.flagged_f1 = 2.0 * report.flagged_precision * report.flagged_recall /
                            (report.flagged_precision + report.flagged_recall);
    }

    return report;
}

} // namespace biopic::evaluation
