#pragma once

#include "biopic/types.hpp"

namespace biopic::cli {

constexpr int kExitAllow = 0;
constexpr int kExitReview = 1;
constexpr int kExitBlock = 2;
constexpr int kExitError = 10;

[[nodiscard]] int exit_code_for_decision(ModerationDecision decision);

} // namespace biopic::cli
