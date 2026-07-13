#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "biopic/fingerprint.hpp"
#include "biopic/types.hpp"

namespace biopic {

constexpr double kDefaultSimilarityThreshold = 10.0;

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

class FingerprintStore {
  public:
    [[nodiscard]] bool add(FingerprintRecord record);

    [[nodiscard]] std::optional<FingerprintRecord> find_exact(
        const Fingerprint& fingerprint) const;

    [[nodiscard]] std::vector<FingerprintRecord> find_similar(const Fingerprint& fingerprint,
                                                              double threshold) const;

    [[nodiscard]] NearestMatchResult find_nearest(const Fingerprint& fingerprint) const;

    [[nodiscard]] std::size_t size() const noexcept { return records_.size(); }

  private:
    [[nodiscard]] std::string generate_id();

    std::vector<FingerprintRecord> records_;
    std::size_t next_id_ = 1;
};

[[nodiscard]] FingerprintStore& shared_fingerprint_store();

[[nodiscard]] bool fingerprints_equal(const Fingerprint& left, const Fingerprint& right);

} // namespace biopic
