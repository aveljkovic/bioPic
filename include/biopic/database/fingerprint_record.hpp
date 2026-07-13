#pragma once

#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "biopic/fingerprint.hpp"
#include "biopic/types.hpp"

namespace biopic {

struct FingerprintRecord {
    std::string id;
    Fingerprint fingerprint;
    std::string label;
    std::chrono::system_clock::time_point created_at = std::chrono::system_clock::now();
};

struct NearestMatchResult {
    bool exact_match = false;
    double distance = 0.0;
    std::optional<FingerprintRecord> record;
};

} // namespace biopic
