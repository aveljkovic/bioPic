#include <gtest/gtest.h>

#include <fstream>

#include "biopic/ai/classifier.hpp"
#include "biopic/ai/classifier_config.hpp"
#include "biopic/ai/runtime.hpp"

#include "test_images.hpp"

namespace {

std::filesystem::path fixture_path(const char* name) {
    return std::filesystem::path(BIOPIC_TEST_FIXTURES_DIR) / name;
}

} // namespace

TEST(OnnxRuntimeTest, EnvironmentInitializes) {
    biopic::OnnxRuntimeEnvironment environment;
    EXPECT_TRUE(environment.is_initialized());
    EXPECT_TRUE(environment.last_error().empty());
}

TEST(OnnxRuntimeTest, LoadMissingModelFails) {
    biopic::OnnxRuntimeEnvironment environment;
    biopic::Model model("missing", std::filesystem::path("definitely_missing_model.onnx"),
                        biopic::ModelMetadata{});
    biopic::OnnxInferenceSession session(environment, std::move(model));

    EXPECT_FALSE(session.load());
    EXPECT_FALSE(session.is_loaded());
    EXPECT_EQ(session.last_error(), "model file does not exist");
}

TEST(OnnxRuntimeTest, LoadInvalidModelBytesFails) {
    const std::filesystem::path invalid_model =
        std::filesystem::temp_directory_path() / "biopic_invalid_model.onnx";
    {
        std::ofstream output(invalid_model);
        output << "this is not a valid onnx model";
    }

    biopic::OnnxRuntimeEnvironment environment;
    const auto loaded = biopic::Model::load_from_path(invalid_model);
    ASSERT_TRUE(loaded.has_value());
    biopic::OnnxInferenceSession session(environment, *loaded);

    EXPECT_FALSE(session.load());
    EXPECT_FALSE(session.is_loaded());
    EXPECT_FALSE(session.last_error().empty());

    std::error_code error;
    std::filesystem::remove(invalid_model, error);
}

TEST(OnnxRuntimeTest, RunBeforeLoadReportsError) {
    biopic::OnnxRuntimeEnvironment environment;
    biopic::Model model = biopic::Model::placeholder("not-loaded");
    biopic::OnnxInferenceSession session(environment, std::move(model));

    biopic::PreparedTensor tensor;
    tensor.width = 1;
    tensor.height = 1;
    tensor.channels = 1;
    tensor.data = {0.5f};

    EXPECT_FALSE(session.run(tensor).has_value());
    EXPECT_EQ(session.last_error(), "inference session is not loaded");
}

TEST(OnnxRuntimeTest, IdentityModelRunsInference) {
    const std::filesystem::path model_path = fixture_path("identity.onnx");
    const std::filesystem::path manifest_path = fixture_path("identity.biopic.manifest");
    ASSERT_TRUE(std::filesystem::exists(model_path));

    biopic::OnnxRuntimeEnvironment environment;
    const auto loaded = biopic::Model::load_from_manifest(model_path, manifest_path);
    ASSERT_TRUE(loaded.has_value());

    biopic::OnnxInferenceSession session(environment, *loaded);
    ASSERT_TRUE(session.load()) << session.last_error();
    EXPECT_TRUE(session.is_loaded());

    const auto image = biopic::test_support::make_uniform_rgb(4, 4, 10, 20, 30);
    biopic::ImageView view(image.width, image.height, image.rgb);
    biopic::Preprocessor preprocessor(loaded->input_requirements());
    const auto tensor = preprocessor.prepare(view);
    ASSERT_TRUE(tensor.has_value());

    const auto output = session.run(*tensor);
    ASSERT_TRUE(output.has_value()) << session.last_error();
    EXPECT_EQ(output->values.size(), tensor->data.size());
    EXPECT_NEAR(output->values.front(), tensor->data.front(), 1e-5f);
}

TEST(OnnxRuntimeTest, OnnxClassifierReportsLoadFailureForMissingModel) {
    biopic::Model model("missing", std::filesystem::path("missing_classifier_model.onnx"),
                        biopic::ModelMetadata{});
    biopic::OnnxClassifier classifier(biopic::ClassifierKind::Nudity, std::move(model));

    const auto image = biopic::test_support::make_uniform_rgb(8, 8, 255, 0, 0);
    biopic::ImageView view(image.width, image.height, image.rgb);
    const auto result = classifier.classify(view);

    EXPECT_EQ(result.label, "model_load_failed");
    EXPECT_FALSE(result.detected);
}

TEST(OnnxRuntimeTest, OnnxClassifierMapsIdentityOutputToManifestLabels) {
    const auto config = biopic::ClassifierConfig::load_from_file(fixture_path("classifier.conf"));
    ASSERT_TRUE(config.has_value());

    const auto classifier = biopic::create_classifier(*config);
    ASSERT_NE(classifier, nullptr);

    const auto image = biopic::test_support::make_uniform_rgb(4, 4, 10, 20, 30);
    biopic::ImageView view(image.width, image.height, image.rgb);
    const auto result = classifier->classify(view);

    EXPECT_EQ(result.label, "channel_b");
    EXPECT_NEAR(result.confidence, 30.0 / 255.0, 1e-4);
    EXPECT_FALSE(result.detected);
}
