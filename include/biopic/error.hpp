#pragma once

#include <string>

namespace biopic {

[[nodiscard]] std::string last_error_message();

void set_last_error(std::string message);

} // namespace biopic
