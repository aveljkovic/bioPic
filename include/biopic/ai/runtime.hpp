#pragma once

#include <memory>
#include <string>

#include "biopic/ai/inference_types.hpp"
#include "biopic/ai/model.hpp"
#include "biopic/ai/preprocessing.hpp"

namespace biopic {

// Owns the process-wide ONNX Runtime environment (Ort::Env).
class OnnxRuntimeEnvironment {
  public:
    OnnxRuntimeEnvironment();
    explicit OnnxRuntimeEnvironment(RuntimeOptions options);
    ~OnnxRuntimeEnvironment();
    OnnxRuntimeEnvironment(OnnxRuntimeEnvironment&&) noexcept;
    OnnxRuntimeEnvironment& operator=(OnnxRuntimeEnvironment&&) noexcept;
    OnnxRuntimeEnvironment(const OnnxRuntimeEnvironment&) = delete;
    OnnxRuntimeEnvironment& operator=(const OnnxRuntimeEnvironment&) = delete;

    [[nodiscard]] bool is_initialized() const noexcept;
    [[nodiscard]] const std::string& last_error() const noexcept;

  private:
    friend class OnnxInferenceSession;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Manages a single ONNX Runtime inference session for one model file.
class OnnxInferenceSession {
  public:
    OnnxInferenceSession(const OnnxRuntimeEnvironment& environment, Model model);
    ~OnnxInferenceSession();
    OnnxInferenceSession(OnnxInferenceSession&&) noexcept;
    OnnxInferenceSession& operator=(OnnxInferenceSession&&) noexcept;
    OnnxInferenceSession(const OnnxInferenceSession&) = delete;
    OnnxInferenceSession& operator=(const OnnxInferenceSession&) = delete;

    [[nodiscard]] bool load();
    void unload() noexcept;
    [[nodiscard]] bool is_loaded() const noexcept;
    [[nodiscard]] const Model& model() const noexcept;
    [[nodiscard]] std::optional<InferenceOutput> run(const PreparedTensor& input);
    [[nodiscard]] const std::string& last_error() const noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace biopic
