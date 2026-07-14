#include "core/parse_double.hpp"

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <string>

namespace biopic::detail {

std::optional<double> parse_double(std::string_view value) {
    if (value.empty()) {
        return std::nullopt;
    }

    const unsigned char first = static_cast<unsigned char>(value.front());
    const unsigned char last = static_cast<unsigned char>(value.back());
    if (std::isspace(first) || std::isspace(last)) {
        return std::nullopt;
    }

    const std::string text(value);
    errno = 0;
    char* end = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);
    if (end == text.c_str() || end != text.c_str() + static_cast<std::ptrdiff_t>(text.size())) {
        return std::nullopt;
    }
    if (errno == ERANGE) {
        return std::nullopt;
    }
    return parsed;
}

} // namespace biopic::detail
