#include "biopic/error.hpp"

namespace biopic {

namespace {
thread_local std::string g_last_error;
} // namespace

std::string last_error_message() { return g_last_error; }

const char* last_error_c_str() noexcept { return g_last_error.c_str(); }

void set_last_error(std::string message) { g_last_error = std::move(message); }

} // namespace biopic
