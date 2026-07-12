#include "biopic/ai/classification_result.hpp"

namespace biopic {

ClassificationResult ClassificationResult::not_detected(std::string label) {
    return ClassificationResult{std::move(label), 0.0, false};
}

} // namespace biopic
