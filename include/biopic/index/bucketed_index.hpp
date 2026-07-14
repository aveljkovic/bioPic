#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "biopic/database/fingerprint_record.hpp"
#include "biopic/fingerprint.hpp"
#include "biopic/index/similarity_index.hpp"
#include "biopic/types.hpp"

namespace biopic {

struct BucketedIndexImpl;

class BucketedIndex final : public SimilarityIndex {
  public:
    BucketedIndex();
    ~BucketedIndex() override;

    [[nodiscard]] bool add(const FingerprintRecord& record) override;
    [[nodiscard]] std::vector<SimilarityQueryResult> query(
        const Fingerprint& fingerprint, const HashMatchConfig& config,
        std::size_t limit = 0) const override;
    [[nodiscard]] NearestMatchResult find_nearest(const Fingerprint& fingerprint,
                                                  const HashMatchConfig& config) const override;
    [[nodiscard]] std::size_t size() const noexcept override;

    [[nodiscard]] std::size_t count_query_candidates(const Fingerprint& fingerprint,
                                                     const HashMatchConfig& config,
                                                     bool for_nearest) const;

  private:
    std::unique_ptr<BucketedIndexImpl> impl_;
};

[[nodiscard]] std::unique_ptr<SimilarityIndex> create_bucketed_index();

} // namespace biopic
