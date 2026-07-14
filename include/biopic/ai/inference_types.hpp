#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace biopic {

struct InferenceOutput {
    std::vector<std::int64_t> shape;
    std::vector<float> values;
};

struct RuntimeOptions {
    int intra_op_num_threads = 1;
    bool enable_cpu_mem_arena = true;
};

} // namespace biopic
