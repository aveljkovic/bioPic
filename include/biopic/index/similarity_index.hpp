#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "biopic/database/fingerprint_record.hpp"
#include "biopic/fingerprint.hpp"
#include "biopic/index/fingerprint_normalization.hpp"
#include "biopic/types.hpp"

namespace biopic {

struct SimilarityQueryResult {
    FingerprintRecord matched;
    double distance = 0.0;
};

class SimilarityIndex {
  public:
    virtual ~SimilarityIndex() = default;

    [[nodiscard]] virtual bool add(const FingerprintRecord& record) = 0;

    [[nodiscard]] virtual std::vector<SimilarityQueryResult> query(
        const Fingerprint& fingerprint, const HashMatchConfig& config,
        std::size_t limit = 0) const = 0;

    [[nodiscard]] virtual NearestMatchResult find_nearest(
        const Fingerprint& fingerprint, const HashMatchConfig& config) const = 0;

    [[nodiscard]] virtual std::size_t size() const = 0;
};

class BruteForceIndex final : public SimilarityIndex {
  public:
    [[nodiscard]] bool add(const FingerprintRecord& record) override;
    [[nodiscard]] std::vector<SimilarityQueryResult> query(
        const Fingerprint& fingerprint, const HashMatchConfig& config,
        std::size_t limit = 0) const override;
    [[nodiscard]] NearestMatchResult find_nearest(const Fingerprint& fingerprint,
                                                  const HashMatchConfig& config) const override;
    [[nodiscard]] std::size_t size() const noexcept override;

  private:
    std::vector<FingerprintRecord> records_;
};

[[nodiscard]] std::unique_ptr<SimilarityIndex> create_brute_force_index();

} // namespace biopic
