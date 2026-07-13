#pragma once

#include <filesystem>

#include "biopic/database/fingerprint_store.hpp"

namespace biopic {

class PersistentFingerprintStore final : public FingerprintStore {
  public:
    PersistentFingerprintStore();
    ~PersistentFingerprintStore() override;

    PersistentFingerprintStore(const PersistentFingerprintStore&) = delete;
    PersistentFingerprintStore& operator=(const PersistentFingerprintStore&) = delete;

    [[nodiscard]] bool open(const std::filesystem::path& database);
    [[nodiscard]] bool is_open() const noexcept;

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
    [[nodiscard]] std::size_t size() const override;

  private:
    [[nodiscard]] std::vector<FingerprintRecord> load_all_records() const;
    [[nodiscard]] std::vector<FingerprintRecord> load_candidate_records(
        const Fingerprint& fingerprint, const HashMatchConfig& config, bool for_nearest) const;
    [[nodiscard]] bool rebuild_similarity_index();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace biopic
