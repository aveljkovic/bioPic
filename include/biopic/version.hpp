#pragma once

#include <string>

namespace biopic {

struct BuildInfo {
    int major = 0;
    int minor = 0;
    int patch = 0;
    std::string version_string;
    std::string git_commit;
    std::string build_type;
};

[[nodiscard]] BuildInfo build_info();

[[nodiscard]] std::string onnx_runtime_version();

[[nodiscard]] std::string sqlite_version_string();

} // namespace biopic
