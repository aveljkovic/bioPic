#include "biopic/index/fingerprint_bucket.hpp"

#include <cmath>

namespace biopic {

namespace {

constexpr std::uint32_t kBucketSpace = 65536;
constexpr std::uint32_t kFnvOffsetBasis = 2166136261U;
constexpr std::uint32_t kFnvPrime = 16777619U;

std::uint32_t fnv1a32(std::uint32_t hash, std::uint8_t byte) {
    hash ^= static_cast<std::uint32_t>(byte);
    return hash * kFnvPrime;
}

void append_unique_bucket(std::vector<BandBucketRef>& refs, std::uint32_t band_index,
                          std::uint32_t bucket_key) {
    for (const BandBucketRef& existing : refs) {
        if (existing.band_index == band_index && existing.bucket_key == bucket_key) {
            return;
        }
    }
    refs.push_back(BandBucketRef{band_index, bucket_key});
}

std::uint32_t packed_nibble_band_key(const Fingerprint& fingerprint, std::size_t band_index) {
    const std::size_t offset = band_index * kFingerprintBandSize;
    std::uint32_t key = static_cast<std::uint32_t>(band_index * 131U + 17U);
    for (std::size_t group = 0; group < 4; ++group) {
        const std::size_t byte_index = offset + ((group * 5 + band_index) % kFingerprintBandSize);
        key = (key << 4) | (static_cast<std::uint32_t>(fingerprint.bytes[byte_index]) >> 4);
    }
    return key % kBucketSpace;
}

void append_nibble_neighbors(std::vector<BandBucketRef>& refs, std::uint32_t band_index,
                             std::uint32_t bucket_key, std::uint32_t max_delta) {
    append_unique_bucket(refs, band_index, bucket_key);
    for (std::size_t nibble_pos = 0; nibble_pos < 4; ++nibble_pos) {
        const std::size_t shift = (3 - nibble_pos) * 4;
        const std::uint32_t mask = 0xFU << shift;
        const std::uint32_t nibble = (bucket_key >> shift) & 0xFU;
        for (std::uint32_t delta = 1; delta <= max_delta; ++delta) {
            if (nibble >= delta) {
                append_unique_bucket(refs, band_index,
                                     (bucket_key & ~mask) | ((nibble - delta) << shift));
            }
            if (nibble + delta <= 0xFU) {
                append_unique_bucket(refs, band_index,
                                     (bucket_key & ~mask) | ((nibble + delta) << shift));
            }
        }
    }
}

} // namespace

std::uint32_t exact_band_slot(std::size_t band_index) {
    return static_cast<std::uint32_t>(band_index);
}

std::uint32_t nibble_band_slot(std::size_t band_index) {
    return static_cast<std::uint32_t>(kFingerprintBandCount + band_index);
}

std::uint32_t prefix_band_slot() {
    return static_cast<std::uint32_t>(kFingerprintBandCount * 2);
}

std::uint32_t exact_band_bucket_key(const Fingerprint& fingerprint, std::size_t band_index) {
    const std::size_t offset = band_index * kFingerprintBandSize;
    std::uint32_t hash = kFnvOffsetBasis;
    for (std::size_t index = 0; index < kFingerprintBandSize; ++index) {
        hash = fnv1a32(hash, fingerprint.bytes[offset + index]);
    }
    return hash % kBucketSpace;
}

std::uint32_t nibble_band_bucket_key(const Fingerprint& fingerprint, std::size_t band_index) {
    return packed_nibble_band_key(fingerprint, band_index);
}

std::uint32_t prefix_bucket_key(const Fingerprint& fingerprint) {
    std::uint32_t hash = kFnvOffsetBasis;
    for (std::size_t index = 0; index < 16 && index < fingerprint.bytes.size(); ++index) {
        hash = fnv1a32(hash, fingerprint.bytes[index]);
    }
    return hash % kBucketSpace;
}

std::vector<BandBucketRef> band_bucket_refs(const Fingerprint& fingerprint) {
    std::vector<BandBucketRef> refs;
    refs.reserve(kFingerprintBandSlotCount);

    append_unique_bucket(refs, prefix_band_slot(), prefix_bucket_key(fingerprint));
    for (std::size_t band_index = 0; band_index < kFingerprintBandCount; ++band_index) {
        append_unique_bucket(refs, exact_band_slot(band_index),
                             exact_band_bucket_key(fingerprint, band_index));
        append_unique_bucket(refs, nibble_band_slot(static_cast<std::uint32_t>(band_index)),
                             nibble_band_bucket_key(fingerprint, band_index));
    }
    return refs;
}

std::uint32_t nibble_neighbor_delta(const HashMatchConfig& config) {
    if (config.threshold <= 15.0) {
        return 1;
    }
    if (config.threshold <= 50.0) {
        return 2;
    }
    return 3;
}

std::vector<BandBucketRef> candidate_bucket_refs(const Fingerprint& fingerprint,
                                                 const HashMatchConfig& config,
                                                 bool for_nearest) {
    if (for_nearest) {
        std::vector<BandBucketRef> refs;
        refs.reserve(kFingerprintBandCount + 1);
        append_unique_bucket(refs, prefix_band_slot(), prefix_bucket_key(fingerprint));
        for (std::size_t band_index = 0; band_index < kFingerprintBandCount; ++band_index) {
            append_unique_bucket(refs, exact_band_slot(band_index),
                                 exact_band_bucket_key(fingerprint, band_index));
        }
        return refs;
    }

    std::vector<BandBucketRef> refs;
    refs.reserve(kFingerprintBandCount * 18);

    const std::uint32_t delta = nibble_neighbor_delta(config);
    for (std::size_t band_index = 0; band_index < kFingerprintBandCount; ++band_index) {
        append_nibble_neighbors(refs, nibble_band_slot(band_index),
                                nibble_band_bucket_key(fingerprint, band_index), delta);
    }
    return refs;
}

std::uint64_t bucket_lookup_key(std::uint32_t band_index, std::uint32_t bucket_key) {
    return (static_cast<std::uint64_t>(band_index) << 32) |
           static_cast<std::uint64_t>(bucket_key);
}

} // namespace biopic
