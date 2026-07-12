#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "biopic/ai/model.hpp"

TEST(ModelTest, PlaceholderModelLoadsWithoutPath) {
    biopic::Model model = biopic::Model::placeholder("dummy-nudity");
    EXPECT_EQ(model.name(), "dummy-nudity");
    EXPECT_TRUE(model.path().empty());
    EXPECT_TRUE(model.load());
    EXPECT_TRUE(model.is_loaded());

    model.unload();
    EXPECT_FALSE(model.is_loaded());
}

TEST(ModelTest, LoadFromPathRequiresExistingFile) {
    const auto missing = biopic::Model::load_from_path("definitely_missing_model.onnx");
    EXPECT_FALSE(missing.has_value());
}

TEST(ModelTest, LoadFromPathReadsOptionalMetadataSidecar) {
    const std::filesystem::path model_path =
        std::filesystem::temp_directory_path() / "biopic_test_model.onnx";
    const std::filesystem::path sidecar_path =
        std::filesystem::temp_directory_path() / "biopic_test_model.meta.txt";

    {
        std::ofstream model_file(model_path);
        model_file << "placeholder";
    }
    {
        std::ofstream sidecar_file(sidecar_path);
        sidecar_file << "version=1.2.3\n";
        sidecar_file << "description=test model\n";
        sidecar_file << "author=biopic\n";
    }

    const auto loaded = biopic::Model::load_from_path(model_path);
    ASSERT_TRUE(loaded.has_value());
    biopic::Model model = *loaded;
    EXPECT_EQ(model.name(), "biopic_test_model");
    EXPECT_EQ(model.metadata().version, "1.2.3");
    EXPECT_EQ(model.metadata().description, "test model");
    EXPECT_EQ(model.metadata().attributes.at("author"), "biopic");
    EXPECT_TRUE(model.load());
    EXPECT_TRUE(model.is_loaded());

    std::error_code error;
    std::filesystem::remove(model_path, error);
    std::filesystem::remove(sidecar_path, error);
}

TEST(ModelTest, InputRequirementsExposeDefaultTensorShape) {
    const biopic::Model model = biopic::Model::placeholder("shape-check");
    EXPECT_EQ(model.input_requirements().width, 224);
    EXPECT_EQ(model.input_requirements().height, 224);
    EXPECT_EQ(model.input_requirements().channels, 3);
}
