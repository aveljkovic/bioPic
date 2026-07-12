#include "biopic/ai/classifier.hpp"

namespace biopic {

std::string classifier_kind_to_string(ClassifierKind kind) {
    switch (kind) {
    case ClassifierKind::Nudity:
        return "nudity";
    case ClassifierKind::Violence:
        return "violence";
    case ClassifierKind::Weapons:
        return "weapons";
    case ClassifierKind::Drugs:
        return "drugs";
    case ClassifierKind::ChildSafety:
        return "child_safety";
    }
    return "unknown";
}

ClassificationResult ClassificationResult::not_detected(std::string label) {
    return ClassificationResult{std::move(label), 0.0, false};
}

DummyClassifier::DummyClassifier(ClassifierKind kind)
    : kind_(kind),
      model_(Model::placeholder("dummy-" + classifier_kind_to_string(kind))),
      preprocessor_(model_.input_requirements()) {
    model_.load();
}

ClassifierKind DummyClassifier::kind() const { return kind_; }

const Model& DummyClassifier::model() const { return model_; }

ClassificationResult DummyClassifier::classify(const ImageView& image) {
    if (image.empty()) {
        return ClassificationResult::not_detected("invalid_input");
    }

    const auto tensor = preprocessor_.prepare(image);
    if (!tensor.has_value()) {
        return ClassificationResult::not_detected("preprocessing_failed");
    }

    (void)tensor;
    return ClassificationResult::not_detected("safe");
}

} // namespace biopic
