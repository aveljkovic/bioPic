#include <gtest/gtest.h>

#include <fstream>

#include "biopic/ai/classifier_config.hpp"
#include "biopic/ai/model.hpp"

namespace {

std::filesystem::path fixture_path(const char* name) {
    return std::filesystem::path(BIOPIC_TEST_FIXTURES_DIR) / name;
}

} // namespace

TEST(ClassifierConfigTest, LoadFromFileResolvesRelativeModelPaths) {
    const auto config_path = fixture_path("classifier.conf");
    ASSERT_TRUE(std::filesystem::exists(config_path));

    const auto config = biopic::ClassifierConfig::load_from_file(config_path);
    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->kind, biopic::ClassifierKind::Nudity);
    EXPECT_EQ(config->model_path, fixture_path("identity.onnx"));
    EXPECT_EQ(config->manifest_path, fixture_path("identity.biopic.manifest"));
}

TEST(ClassifierConfigTest, CreateClassifierBuildsOnnxClassifier) {
    const auto config_path = fixture_path("classifier.conf");
    const auto config = biopic::ClassifierConfig::load_from_file(config_path);
    ASSERT_TRUE(config.has_value());

    const auto classifier = biopic::create_classifier(*config);
    ASSERT_NE(classifier, nullptr);
    EXPECT_EQ(classifier->kind(), biopic::ClassifierKind::Nudity);
    EXPECT_EQ(classifier->model().name(), "identity-test");
}

TEST(ClassifierConfigTest, MissingModelPathReturnsNullClassifier) {
    const std::filesystem::path config_path =
        std::filesystem::temp_directory_path() / "biopic_missing_model.conf";
    {
        std::ofstream output(config_path);
        output << "kind=nudity\n";
    }

    const auto config = biopic::ClassifierConfig::load_from_file(config_path);
    EXPECT_FALSE(config.has_value());

    std::error_code error;
    std::filesystem::remove(config_path, error);
}

TEST(ModelManifestTest, ParseManifestPopulatesMetadataAndNormalization) {
    const auto manifest_path = fixture_path("identity.biopic.manifest");
    const auto metadata = biopic::parse_model_manifest(manifest_path);
    ASSERT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->name, "identity-test");
    EXPECT_EQ(metadata->version, "1.0.0");
    EXPECT_EQ(metadata->input_requirements.width, 4);
    EXPECT_EQ(metadata->input_requirements.height, 4);
    EXPECT_EQ(metadata->input_requirements.channels, 3);
    EXPECT_NEAR(metadata->input_requirements.scale, 1.0 / 255.0, 1e-9);
    ASSERT_EQ(metadata->output_mapping.labels.size(), 3U);
    EXPECT_EQ(metadata->output_mapping.labels[0], "channel_r");
    EXPECT_EQ(metadata->output_mapping.detection_label, "channel_r");
    EXPECT_DOUBLE_EQ(metadata->output_mapping.detection_threshold, 1.0);
}

TEST(ModelManifestTest, LoadFromManifestAppliesInputRequirements) {
    const auto model_path = fixture_path("identity.onnx");
    const auto manifest_path = fixture_path("identity.biopic.manifest");
    const auto model = biopic::Model::load_from_manifest(model_path, manifest_path);
    ASSERT_TRUE(model.has_value());
    EXPECT_EQ(model->input_requirements().width, 4);
    EXPECT_EQ(model->input_requirements().height, 4);
    EXPECT_EQ(model->output_mapping().labels.size(), 3U);
}
