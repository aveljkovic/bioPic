#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
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
    virtual ~FingerprintStore() = default;

    [[nodiscard]] virtual bool add(FingerprintRecord record) = 0;

    [[nodiscard]] virtual std::optional<FingerprintRecord> find_exact(
        const Fingerprint& fingerprint) const = 0;

    [[nodiscard]] virtual std::vector<FingerprintRecord> find_similar(
        const Fingerprint& fingerprint, double threshold) const = 0;

    [[nodiscard]] virtual NearestMatchResult find_nearest(
        const Fingerprint& fingerprint) const = 0;

    [[nodiscard]] virtual std::size_t size() const = 0;
};

class InMemoryFingerprintStore final : public FingerprintStore {
  public:
    [[nodiscard]] bool add(FingerprintRecord record) override;
    [[nodiscard]] std::optional<FingerprintRecord> find_exact(
        const Fingerprint& fingerprint) const override;
    [[nodiscard]] std::vector<FingerprintRecord> find_similar(const Fingerprint& fingerprint,
                                                              double threshold) const override;
    [[nodiscard]] NearestMatchResult find_nearest(const Fingerprint& fingerprint) const override;
    [[nodiscard]] std::size_t size() const noexcept override { return records_.size(); }

  private:
    [[nodiscard]] std::string generate_id();

    std::vector<FingerprintRecord> records_;
    std::size_t next_id_ = 1;
};

[[nodiscard]] std::unique_ptr<FingerprintStore> open_persistent_fingerprint_store(
    const std::filesystem::path& database_path);

[[nodiscard]] bool fingerprints_equal(const Fingerprint& left, const Fingerprint& right);

[[nodiscard]] NearestMatchResult find_nearest_among_records(
    const std::vector<FingerprintRecord>& records, const Fingerprint& fingerprint);

[[nodiscard]] std::vector<FingerprintRecord> find_similar_among_records(
    const std::vector<FingerprintRecord>& records, const Fingerprint& fingerprint,
    double threshold);

} // namespace biopic
