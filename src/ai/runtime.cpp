#include "biopic/ai/runtime.hpp"

#include "biopic/ai/inference_types.hpp"

#include <onnxruntime_cxx_api.h>

#include <array>
#include <algorithm>
#include <filesystem>
#include <utility>

namespace biopic {

namespace {

#ifdef _WIN32
std::wstring path_to_wide(const std::filesystem::path& path) { return path.wstring(); }
#else
std::string path_to_native(const std::filesystem::path& path) { return path.string(); }
#endif

} // namespace

struct OnnxRuntimeEnvironment::Impl {
    std::unique_ptr<Ort::Env> env;
    RuntimeOptions options;
    std::string last_error;
    bool initialized = false;

    explicit Impl(RuntimeOptions runtime_options) : options(runtime_options) {
        try {
            env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "biopic");
            initialized = true;
        } catch (const Ort::Exception& exception) {
            last_error = exception.what();
            initialized = false;
        }
    }
};

OnnxRuntimeEnvironment::OnnxRuntimeEnvironment() : OnnxRuntimeEnvironment(RuntimeOptions{}) {}

OnnxRuntimeEnvironment::OnnxRuntimeEnvironment(RuntimeOptions options)
    : impl_(std::make_unique<Impl>(options)) {}

OnnxRuntimeEnvironment::~OnnxRuntimeEnvironment() = default;

OnnxRuntimeEnvironment::OnnxRuntimeEnvironment(OnnxRuntimeEnvironment&&) noexcept = default;

OnnxRuntimeEnvironment& OnnxRuntimeEnvironment::operator=(OnnxRuntimeEnvironment&&) noexcept =
    default;

bool OnnxRuntimeEnvironment::is_initialized() const noexcept {
    return impl_ != nullptr && impl_->initialized;
}

const std::string& OnnxRuntimeEnvironment::last_error() const noexcept {
    static const std::string kEmpty;
    return impl_ != nullptr ? impl_->last_error : kEmpty;
}

struct OnnxInferenceSession::Impl {
    const OnnxRuntimeEnvironment* environment = nullptr;
    Model model;
    std::unique_ptr<Ort::Session> session;
    std::string input_name;
    std::string output_name;
    std::string last_error;
    bool loaded = false;

    void unload() noexcept {
        session.reset();
        loaded = false;
        model.unload();
    }

    Impl(const OnnxRuntimeEnvironment& environment_ref, Model model_descriptor)
        : environment(&environment_ref), model(std::move(model_descriptor)) {}
};

OnnxInferenceSession::OnnxInferenceSession(const OnnxRuntimeEnvironment& environment, Model model)
    : impl_(std::make_unique<Impl>(environment, std::move(model))) {}

OnnxInferenceSession::~OnnxInferenceSession() = default;

OnnxInferenceSession::OnnxInferenceSession(OnnxInferenceSession&&) noexcept = default;

OnnxInferenceSession& OnnxInferenceSession::operator=(OnnxInferenceSession&&) noexcept = default;

bool OnnxInferenceSession::load() {
    if (impl_ == nullptr) {
        return false;
    }

    impl_->last_error.clear();
    impl_->unload();

    if (!impl_->environment->is_initialized() || impl_->environment->impl_->env == nullptr) {
        impl_->last_error = "ONNX runtime environment is not initialized";
        return false;
    }

    if (impl_->model.path().empty()) {
        impl_->last_error = "model path is empty";
        return false;
    }

    std::error_code error;
    if (!std::filesystem::exists(impl_->model.path(), error) || error) {
        impl_->last_error = "model file does not exist";
        return false;
    }

    try {
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(
            std::max(1, impl_->environment->impl_->options.intra_op_num_threads));
        if (!impl_->environment->impl_->options.enable_cpu_mem_arena) {
            session_options.DisableCpuMemArena();
        }
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

#ifdef _WIN32
        impl_->session = std::make_unique<Ort::Session>(*impl_->environment->impl_->env,
                                                        path_to_wide(impl_->model.path()).c_str(),
                                                        session_options);
#else
        impl_->session = std::make_unique<Ort::Session>(*impl_->environment->impl_->env,
                                                        path_to_native(impl_->model.path()).c_str(),
                                                        session_options);
#endif

        Ort::AllocatorWithDefaultOptions allocator;
        {
            Ort::AllocatedStringPtr input_name =
                impl_->session->GetInputNameAllocated(0, allocator);
            impl_->input_name = input_name.get();
        }
        {
            Ort::AllocatedStringPtr output_name =
                impl_->session->GetOutputNameAllocated(0, allocator);
            impl_->output_name = output_name.get();
        }

        if (impl_->input_name.empty() || impl_->output_name.empty()) {
            impl_->last_error = "model is missing input or output metadata";
            impl_->session.reset();
            return false;
        }

        impl_->loaded = true;
        impl_->model.load();
        return true;
    } catch (const Ort::Exception& exception) {
        impl_->last_error = exception.what();
        impl_->session.reset();
        impl_->loaded = false;
        return false;
    }
}

void OnnxInferenceSession::unload() noexcept {
    if (impl_ != nullptr) {
        impl_->unload();
    }
}

bool OnnxInferenceSession::is_loaded() const noexcept {
    return impl_ != nullptr && impl_->loaded;
}

const Model& OnnxInferenceSession::model() const noexcept {
    static const Model kEmpty = Model::placeholder("uninitialized");
    return impl_ != nullptr ? impl_->model : kEmpty;
}

std::optional<InferenceOutput> OnnxInferenceSession::run(const PreparedTensor& input) {
    if (impl_ == nullptr) {
        return std::nullopt;
    }

    impl_->last_error.clear();

    if (!impl_->loaded || impl_->session == nullptr) {
        impl_->last_error = "inference session is not loaded";
        return std::nullopt;
    }

    if (input.data.empty() || input.width <= 0 || input.height <= 0 || input.channels <= 0) {
        impl_->last_error = "prepared tensor is empty";
        return std::nullopt;
    }

    try {
        const std::array<std::int64_t, 4> input_shape = {1, input.channels, input.height,
                                                         input.width};
        const Ort::MemoryInfo memory_info =
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, const_cast<float*>(input.data.data()), input.data.size(),
            input_shape.data(), input_shape.size());

        const char* input_names[] = {impl_->input_name.c_str()};
        const char* output_names[] = {impl_->output_name.c_str()};

        auto output_tensors = impl_->session->Run(Ort::RunOptions{nullptr}, input_names,
                                                  &input_tensor, 1, output_names, 1);
        if (output_tensors.empty() || !output_tensors.front().IsTensor()) {
            impl_->last_error = "model returned no tensor output";
            return std::nullopt;
        }

        Ort::Value& output_tensor = output_tensors.front();
        float* output_data = output_tensor.GetTensorMutableData<float>();
        const auto output_shape = output_tensor.GetTensorTypeAndShapeInfo().GetShape();
        const std::size_t element_count =
            output_tensor.GetTensorTypeAndShapeInfo().GetElementCount();

        InferenceOutput output;
        output.shape.assign(output_shape.begin(), output_shape.end());
        output.values.assign(output_data, output_data + element_count);
        return output;
    } catch (const Ort::Exception& exception) {
        impl_->last_error = exception.what();
        return std::nullopt;
    }
}

const std::string& OnnxInferenceSession::last_error() const noexcept {
    static const std::string kEmpty;
    return impl_ != nullptr ? impl_->last_error : kEmpty;
}

} // namespace biopic
