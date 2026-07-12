#pragma once

#include <string>

#include "biopic/ai/classifier.hpp"
#include "biopic/image.hpp"
#include "biopic/types.hpp"

namespace biopic {

struct NudityClassificationResult {
    ClassificationScores scores;
    std::string model_id;
    std::string model_version;
    std::string model_sha256;
};

// Abstract interface for ONNX-backed nudity classification.
// Concrete implementations will implement IClassifier in a future milestone.
class INudityClassifier {
  public:
    virtual ~INudityClassifier() = default;
    virtual NudityClassificationResult classify(const ImageView& image) = 0;
};

} // namespace biopic
