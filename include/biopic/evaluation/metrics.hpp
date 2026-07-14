#pragma once

#include <cstddef>

#include "biopic/types.hpp"

namespace biopic::evaluation {

struct ClassMetrics {
    double precision = 0.0;
    double recall = 0.0;
    double f1 = 0.0;
    std::size_t support = 0;
};

struct ConfusionMatrix {
    std::size_t allow_allow = 0;
    std::size_t allow_review = 0;
    std::size_t allow_block = 0;
    std::size_t review_allow = 0;
    std::size_t review_review = 0;
    std::size_t review_block = 0;
    std::size_t block_allow = 0;
    std::size_t block_review = 0;
    std::size_t block_block = 0;
};

struct EvaluationReport {
    std::size_t total = 0;
    std::size_t evaluated = 0;
    std::size_t skipped = 0;
    double accuracy = 0.0;
    ClassMetrics allow;
    ClassMetrics review;
    ClassMetrics block;
    double flagged_precision = 0.0;
    double flagged_recall = 0.0;
    double flagged_f1 = 0.0;
    ConfusionMatrix confusion;
};

void record_prediction(ConfusionMatrix& matrix, ModerationDecision expected,
                       ModerationDecision predicted);

[[nodiscard]] EvaluationReport compute_report(const ConfusionMatrix& matrix,
                                              std::size_t skipped);

} // namespace evaluation
