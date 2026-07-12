#pragma once

#include <string>

#include "biopic/image.hpp"
#include "biopic/types.hpp"

namespace biopic {

struct ClassificationResult {
    ClassificationScores scores;
    std::string model_id;
    std::string model_version;
    std::string model_sha256;
};

// Abstract interface for ONNX-backed nudity classification.
// No concrete implementation is provided in Milestone 1.
class INudityClassifier {
  public:
    virtual ~INudityClassifier() = default;
    virtual ClassificationResult classify(const ImageView& image) = 0;
};

} // namespace biopic
