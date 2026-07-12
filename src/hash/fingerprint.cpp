#include "biopic/fingerprint.hpp"

namespace biopic {

bool Fingerprint::is_zero() const noexcept {
    for (const auto byte : bytes) {
        if (byte != 0) {
            return false;
        }
    }
    return true;
}

} // namespace biopic
