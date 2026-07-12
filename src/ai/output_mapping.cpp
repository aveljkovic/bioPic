#include "biopic/ai/output_mapping.hpp"

#include "biopic/ai/classification_result.hpp"

#include <algorithm>
#include <cmath>

namespace biopic {

ClassificationResult OutputMapping::map_scores(const std::vector<float>& scores) const {
    if (labels.empty()) {
        return ClassificationResult::not_detected("missing_labels");
    }
    if (scores.empty()) {
        return ClassificationResult::not_detected("empty_output");
    }
    if (scores.size() != labels.size()) {
        return ClassificationResult::not_detected("output_shape_mismatch");
    }

    const auto best_index = static_cast<std::size_t>(
        std::distance(scores.begin(), std::max_element(scores.begin(), scores.end())));
    const std::string& label = labels[best_index];
    const double confidence = static_cast<double>(scores[best_index]);
    const bool detected =
        !detection_label.empty() && label == detection_label && confidence >= detection_threshold;
    return ClassificationResult{label, confidence, detected};
}

ClassificationResult OutputMapping::map(const InferenceOutput& output) const {
    if (output.values.empty()) {
        return ClassificationResult::not_detected("empty_output");
    }

    if (!labels.empty() && output.values.size() == labels.size()) {
        return map_scores(output.values);
    }

    if (!labels.empty() && output.values.size() % labels.size() == 0) {
        const std::size_t samples_per_label = output.values.size() / labels.size();
        std::vector<float> aggregated(labels.size(), 0.0f);
        for (std::size_t label_index = 0; label_index < labels.size(); ++label_index) {
            const std::size_t begin = label_index * samples_per_label;
            float sum = 0.0f;
            for (std::size_t offset = 0; offset < samples_per_label; ++offset) {
                sum += output.values[begin + offset];
            }
            aggregated[label_index] = sum / static_cast<float>(samples_per_label);
        }
        return map_scores(aggregated);
    }

    return ClassificationResult::not_detected("output_shape_mismatch");
}

} // namespace biopic
