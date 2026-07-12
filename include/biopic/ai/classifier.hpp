#pragma once

#include <string>

#include "biopic/ai/model.hpp"
#include "biopic/ai/preprocessing.hpp"
#include "biopic/image.hpp"

namespace biopic {

enum class ClassifierKind {
    Nudity,
    Violence,
    Weapons,
    Drugs,
    ChildSafety,
};

[[nodiscard]] std::string classifier_kind_to_string(ClassifierKind kind);

struct ClassificationResult {
    std::string label;
    double confidence = 0.0;
    bool detected = false;

    [[nodiscard]] static ClassificationResult not_detected(std::string label = "safe");
};

// Abstract image classifier interface for future ONNX-backed models.
class IClassifier {
  public:
    virtual ~IClassifier() = default;

    [[nodiscard]] virtual ClassifierKind kind() const = 0;
    [[nodiscard]] virtual const Model& model() const = 0;
    [[nodiscard]] virtual ClassificationResult classify(const ImageView& image) = 0;
};

// Placeholder classifier that exercises preprocessing without running inference.
class DummyClassifier final : public IClassifier {
  public:
    explicit DummyClassifier(ClassifierKind kind = ClassifierKind::Nudity);

    [[nodiscard]] ClassifierKind kind() const override;
    [[nodiscard]] const Model& model() const override;
    [[nodiscard]] ClassificationResult classify(const ImageView& image) override;

  private:
    ClassifierKind kind_;
    Model model_;
    Preprocessor preprocessor_;
};

} // namespace biopic
