#pragma once

#include <string>
#include <vector>

#include "biopic/ai/classification_result.hpp"
#include "biopic/ai/inference_types.hpp"

namespace biopic {

// Maps raw ONNX logits/scores to a ClassificationResult using manifest labels.
struct OutputMapping {
    std::vector<std::string> labels;
    std::string detection_label;
    double detection_threshold = 0.5;

    [[nodiscard]] ClassificationResult map(const InferenceOutput& output) const;
    [[nodiscard]] ClassificationResult map_scores(const std::vector<float>& scores) const;
};

} // namespace biopic
