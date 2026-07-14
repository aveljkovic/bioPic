#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "biopic/database/fingerprint_record.hpp"
#include "biopic/fingerprint.hpp"
#include "biopic/types.hpp"

namespace biopic {

class SimilarityIndex;

constexpr double kDefaultSimilarityThreshold = kDefaultHashMatchConfig.threshold;

class FingerprintStore {
  public:
    virtual ~FingerprintStore() = default;

    [[nodiscard]] virtual bool add(FingerprintRecord record) = 0;

    [[nodiscard]] virtual std::optional<FingerprintRecord> find_exact(
        const Fingerprint& fingerprint) const = 0;

    [[nodiscard]] virtual std::vector<FingerprintRecord> find_similar(
        const Fingerprint& fingerprint, double threshold) const = 0;

    [[nodiscard]] virtual std::vector<FingerprintRecord> find_similar(
        const Fingerprint& fingerprint, const HashMatchConfig& config) const = 0;

    [[nodiscard]] virtual NearestMatchResult find_nearest(
        const Fingerprint& fingerprint) const = 0;

    [[nodiscard]] virtual NearestMatchResult find_nearest(
        const Fingerprint& fingerprint, const HashMatchConfig& config) const = 0;

    [[nodiscard]] virtual std::size_t size() const = 0;
};

class InMemoryFingerprintStore final : public FingerprintStore {
  public:
    InMemoryFingerprintStore();
    ~InMemoryFingerprintStore() override;

    [[nodiscard]] bool add(FingerprintRecord record) override;
    [[nodiscard]] std::optional<FingerprintRecord> find_exact(
        const Fingerprint& fingerprint) const override;
    [[nodiscard]] std::vector<FingerprintRecord> find_similar(const Fingerprint& fingerprint,
                                                              double threshold) const override;
    [[nodiscard]] std::vector<FingerprintRecord> find_similar(
        const Fingerprint& fingerprint, const HashMatchConfig& config) const override;
    [[nodiscard]] NearestMatchResult find_nearest(const Fingerprint& fingerprint) const override;
    [[nodiscard]] NearestMatchResult find_nearest(const Fingerprint& fingerprint,
                                                  const HashMatchConfig& config) const override;
    [[nodiscard]] std::size_t size() const noexcept override;

  private:
    [[nodiscard]] std::string generate_id();

    std::unique_ptr<SimilarityIndex> index_;
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
