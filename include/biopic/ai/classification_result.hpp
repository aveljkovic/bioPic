#pragma once

#include <string>

namespace biopic {

struct ClassificationResult {
    std::string label;
    double confidence = 0.0;
    bool detected = false;

    [[nodiscard]] static ClassificationResult not_detected(std::string label = "safe");
};

} // namespace biopic
