#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "biopic/ai/model.hpp"
#include "biopic/image.hpp"

namespace biopic {

// Channel-first float tensor prepared for future inference backends.
struct PreparedTensor {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<float> data;
};

class Preprocessor {
  public:
    explicit Preprocessor(ModelInputRequirements requirements);

    [[nodiscard]] const ModelInputRequirements& requirements() const noexcept {
        return requirements_;
    }

    [[nodiscard]] std::optional<PreparedTensor> prepare(const ImageView& image) const;

  private:
    ModelInputRequirements requirements_;
};

} // namespace biopic
