#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "biopic/ai/preprocessing.hpp"

namespace biopic {

enum class TensorLayout { NCHW };

enum class ChannelOrder { RGB };

enum class TensorDataType { Float32 };

// Canonical BioPic ONNX input contract shared by all classifiers.
struct InferenceTensorContract {
    TensorLayout layout = TensorLayout::NCHW;
    ChannelOrder channel_order = ChannelOrder::RGB;
    TensorDataType data_type = TensorDataType::Float32;
    int batch_size = 1;

    [[nodiscard]] static InferenceTensorContract biopic_v1() noexcept;
    [[nodiscard]] bool matches(const PreparedTensor& tensor) const noexcept;
    [[nodiscard]] std::optional<std::string> validate(const PreparedTensor& tensor) const;
};

} // namespace biopic
