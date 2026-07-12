#include "biopic/ai/tensor_contract.hpp"

namespace biopic {

InferenceTensorContract InferenceTensorContract::biopic_v1() noexcept {
    InferenceTensorContract contract;
    contract.layout = TensorLayout::NCHW;
    contract.channel_order = ChannelOrder::RGB;
    contract.data_type = TensorDataType::Float32;
    contract.batch_size = 1;
    return contract;
}

bool InferenceTensorContract::matches(const PreparedTensor& tensor) const noexcept {
    return !validate(tensor).has_value();
}

std::optional<std::string> InferenceTensorContract::validate(
    const PreparedTensor& tensor) const {
    if (data_type != TensorDataType::Float32) {
        return std::string{"tensor contract requires float32 input"};
    }
    if (layout != TensorLayout::NCHW) {
        return std::string{"tensor contract requires NCHW layout"};
    }
    if (channel_order != ChannelOrder::RGB) {
        return std::string{"tensor contract requires RGB channel order"};
    }
    if (tensor.width <= 0 || tensor.height <= 0 || tensor.channels <= 0) {
        return std::string{"tensor dimensions must be positive"};
    }
    const std::size_t expected_elements =
        static_cast<std::size_t>(batch_size) * static_cast<std::size_t>(tensor.channels) *
        static_cast<std::size_t>(tensor.height) * static_cast<std::size_t>(tensor.width);
    if (tensor.data.size() != expected_elements) {
        return std::string{"tensor element count does not match NCHW shape"};
    }
    return std::nullopt;
}

} // namespace biopic
