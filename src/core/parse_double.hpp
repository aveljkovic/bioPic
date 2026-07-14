#pragma once

#include <optional>
#include <string_view>

namespace biopic::detail {

[[nodiscard]] std::optional<double> parse_double(std::string_view value);

} // namespace biopic::detail
