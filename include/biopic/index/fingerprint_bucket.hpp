#pragma once

#include <cstdint>
#include <vector>

#include "biopic/fingerprint.hpp"
#include "biopic/types.hpp"

namespace biopic {

constexpr std::size_t kFingerprintBandCount = 12;
constexpr std::size_t kFingerprintBandSize = 12;

struct BandBucketRef {
    std::uint32_t band_index = 0;
    std::uint32_t bucket_key = 0;

    [[nodiscard]] bool operator==(const BandBucketRef& other) const noexcept {
        return band_index == other.band_index && bucket_key == other.bucket_key;
    }
};

[[nodiscard]] std::uint32_t band_bucket_key(const Fingerprint& fingerprint, std::size_t band_index);

[[nodiscard]] std::uint32_t prefix_bucket_key(const Fingerprint& fingerprint);

[[nodiscard]] std::vector<BandBucketRef> band_bucket_refs(const Fingerprint& fingerprint);

[[nodiscard]] std::vector<BandBucketRef> candidate_bucket_refs(const Fingerprint& fingerprint,
                                                               const HashMatchConfig& config,
                                                               bool for_nearest = false);

[[nodiscard]] std::uint64_t bucket_lookup_key(std::uint32_t band_index, std::uint32_t bucket_key);

} // namespace biopic
