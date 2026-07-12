#pragma once

#include <string>

namespace biopic {

[[nodiscard]] std::string last_error_message();

// Pointer remains valid until the next set_last_error() call on this thread.
[[nodiscard]] const char* last_error_c_str() noexcept;

void set_last_error(std::string message);

} // namespace biopic
