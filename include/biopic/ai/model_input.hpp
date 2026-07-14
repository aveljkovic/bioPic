#pragma once

#include <array>

namespace biopic {

struct ModelInputRequirements {
    int width = 224;
    int height = 224;
    int channels = 3;
    double scale = 1.0 / 255.0;
    std::array<float, 3> mean{0.0f, 0.0f, 0.0f};
    std::array<float, 3> stddev{1.0f, 1.0f, 1.0f};
};

} // namespace biopic
