#pragma once

#include <cstdlib>
#include <optional>
#include <string>

namespace biopic {

inline std::optional<std::string> read_env_variable(const char* name) {
#if defined(_MSC_VER)
    char* buffer = nullptr;
    size_t size = 0;
    if (_dupenv_s(&buffer, &size, name) != 0 || buffer == nullptr) {
        free(buffer);
        return std::nullopt;
    }
    if (buffer[0] == '\0') {
        free(buffer);
        return std::nullopt;
    }
    const std::string value(buffer);
    free(buffer);
    return value;
#else
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return std::nullopt;
    }
    return std::string(value);
#endif
}

} // namespace biopic
