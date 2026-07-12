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

TEST(ModelTest, LoadFromManifestReadsNormalizationAndLabels) {
    const std::filesystem::path model_path =
        std::filesystem::temp_directory_path() / "biopic_test_model.onnx";
    const std::filesystem::path manifest_path =
        std::filesystem::temp_directory_path() / "biopic_test_model.biopic.manifest";

    {
        std::ofstream model_file(model_path);
        model_file << "placeholder";
    }
    {
        std::ofstream manifest_file(manifest_path);
        manifest_file << "name=test-model\n";
        manifest_file << "version=1.2.3\n";
        manifest_file << "description=test model\n";
        manifest_file << "input_width=128\n";
        manifest_file << "input_height=96\n";
        manifest_file << "input_channels=3\n";
        manifest_file << "labels=safe,review\n";
        manifest_file << "detection_label=review\n";
        manifest_file << "author=biopic\n";
    }

    const auto loaded = biopic::Model::load_from_manifest(model_path, manifest_path);
    ASSERT_TRUE(loaded.has_value());
    biopic::Model model = *loaded;
    EXPECT_EQ(model.name(), "test-model");
    EXPECT_EQ(model.metadata().version, "1.2.3");
    EXPECT_EQ(model.metadata().description, "test model");
    EXPECT_EQ(model.metadata().attributes.at("author"), "biopic");
    EXPECT_EQ(model.input_requirements().width, 128);
    EXPECT_EQ(model.input_requirements().height, 96);
    EXPECT_EQ(model.output_mapping().labels.size(), 2U);
    EXPECT_TRUE(model.load());
    EXPECT_TRUE(model.is_loaded());

    std::error_code error;
    std::filesystem::remove(model_path, error);
    std::filesystem::remove(manifest_path, error);
}

TEST(ModelTest, InputRequirementsExposeDefaultTensorShape) {
    const biopic::Model model = biopic::Model::placeholder("shape-check");
    EXPECT_EQ(model.input_requirements().width, 224);
    EXPECT_EQ(model.input_requirements().height, 224);
    EXPECT_EQ(model.input_requirements().channels, 3);
}

TEST(ModelTest, MissingManifestReturnsNullopt) {
    const std::filesystem::path model_path =
        std::filesystem::temp_directory_path() / "biopic_manifest_missing.onnx";
    {
        std::ofstream model_file(model_path);
        model_file << "placeholder";
    }

    const auto loaded = biopic::Model::load_from_manifest(
        model_path, std::filesystem::path("missing_manifest.biopic.manifest"));
    EXPECT_FALSE(loaded.has_value());

    std::error_code error;
    std::filesystem::remove(model_path, error);
}
