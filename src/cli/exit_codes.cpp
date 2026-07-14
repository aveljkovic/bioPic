#include "biopic/cli/exit_codes.hpp"

#include "biopic/types.hpp"

namespace biopic::cli {

int exit_code_for_decision(ModerationDecision decision) {
    switch (decision) {
    case ModerationDecision::Allow:
        return kExitAllow;
    case ModerationDecision::Review:
        return kExitReview;
    case ModerationDecision::Block:
        return kExitBlock;
    }
    return kExitError;
}

} // namespace biopic::cli
