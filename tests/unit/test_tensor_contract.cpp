#include <gtest/gtest.h>

#include "biopic/ai/tensor_contract.hpp"

#include "test_images.hpp"

TEST(TensorContractTest, BiopicV1RequiresNchwFloatRgb) {
    const auto contract = biopic::InferenceTensorContract::biopic_v1();
    EXPECT_EQ(contract.layout, biopic::TensorLayout::NCHW);
    EXPECT_EQ(contract.channel_order, biopic::ChannelOrder::RGB);
    EXPECT_EQ(contract.data_type, biopic::TensorDataType::Float32);
    EXPECT_EQ(contract.batch_size, 1);
}

TEST(TensorContractTest, ValidPreparedTensorPassesValidation) {
    const auto image = biopic::test_support::make_uniform_rgb(8, 6, 10, 20, 30);
    biopic::ImageView view(image.width, image.height, image.rgb);

    biopic::ModelInputRequirements requirements;
    requirements.width = 8;
    requirements.height = 6;
    requirements.channels = 3;

    biopic::Preprocessor preprocessor(requirements);
    const auto tensor = preprocessor.prepare(view);
    ASSERT_TRUE(tensor.has_value());

    const auto contract = biopic::InferenceTensorContract::biopic_v1();
    EXPECT_TRUE(contract.matches(*tensor));
    EXPECT_FALSE(contract.validate(*tensor).has_value());
}

TEST(TensorContractTest, WrongElementCountFailsValidation) {
    biopic::PreparedTensor tensor;
    tensor.width = 4;
    tensor.height = 4;
    tensor.channels = 3;
    tensor.data.assign(10, 0.0f);

    const auto contract = biopic::InferenceTensorContract::biopic_v1();
    const auto error = contract.validate(tensor);
    ASSERT_TRUE(error.has_value());
    EXPECT_EQ(*error, "tensor element count does not match NCHW shape");
}
