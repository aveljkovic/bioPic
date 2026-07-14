#include "biopic/ai/classifier.hpp"

#include "biopic/ai/classification_result.hpp"
#include "biopic/ai/tensor_contract.hpp"

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

DummyClassifier::DummyClassifier(ClassifierKind kind)
    : kind_(kind),
      model_(Model::placeholder("dummy-" + classifier_kind_to_string(kind))),
      preprocessor_(model_.input_requirements()) {
    (void)model_.load();
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

    const auto contract_error = model_.metadata().tensor_contract.validate(*tensor);
    if (contract_error.has_value()) {
        return ClassificationResult::not_detected("tensor_contract_violation");
    }

    return ClassificationResult::not_detected("safe");
}

OnnxClassifier::OnnxClassifier(ClassifierKind kind, Model model, RuntimeOptions options)
    : kind_(kind),
      environment_(options),
      preprocessor_(model.input_requirements()),
      session_(environment_, std::move(model)) {}

ClassifierKind OnnxClassifier::kind() const { return kind_; }

const Model& OnnxClassifier::model() const { return session_.model(); }

const OnnxInferenceSession& OnnxClassifier::session() const noexcept { return session_; }

ClassificationResult OnnxClassifier::classify(const ImageView& image) {
    if (image.empty()) {
        return ClassificationResult::not_detected("invalid_input");
    }

    if (!session_.is_loaded() && !session_.load()) {
        return ClassificationResult::not_detected("model_load_failed");
    }

    const auto tensor = preprocessor_.prepare(image);
    if (!tensor.has_value()) {
        return ClassificationResult::not_detected("preprocessing_failed");
    }

    const auto contract_error = model().metadata().tensor_contract.validate(*tensor);
    if (contract_error.has_value()) {
        return ClassificationResult::not_detected("tensor_contract_violation");
    }

    const auto output = session_.run(*tensor);
    if (!output.has_value()) {
        return ClassificationResult::not_detected("inference_failed");
    }

    return model().output_mapping().map(*output);
}

} // namespace biopic
