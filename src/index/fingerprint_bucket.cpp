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

std::uint32_t hash_band(const Fingerprint& fingerprint, std::size_t band_index) {
    const std::size_t offset = band_index * kFingerprintBandSize;
    std::uint32_t hash = kFnvOffsetBasis;
    for (std::size_t index = 0; index < kFingerprintBandSize; ++index) {
        hash = fnv1a32(hash, fingerprint.bytes[offset + index]);
    }
    return hash % kBucketSpace;
}

std::uint32_t neighbor_radius(const HashMatchConfig& config) {
    return static_cast<std::uint32_t>(
        std::min(16.0, std::max(1.0, std::ceil(config.threshold / 2.0))));
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

} // namespace

std::uint32_t band_bucket_key(const Fingerprint& fingerprint, std::size_t band_index) {
    return hash_band(fingerprint, band_index);
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
    refs.reserve(kFingerprintBandCount + 1);
    append_unique_bucket(refs, kFingerprintBandCount, prefix_bucket_key(fingerprint));
    for (std::size_t band_index = 0; band_index < kFingerprintBandCount; ++band_index) {
        append_unique_bucket(refs, static_cast<std::uint32_t>(band_index),
                             band_bucket_key(fingerprint, band_index));
    }
    return refs;
}

std::vector<BandBucketRef> candidate_bucket_refs(const Fingerprint& fingerprint,
                                                 const HashMatchConfig& config,
                                                 bool for_nearest) {
    std::vector<BandBucketRef> refs;
    refs.reserve((kFingerprintBandCount + 1) * 33);

    const auto add_exact_buckets = [&](std::uint32_t band_index, std::uint32_t bucket_key) {
        append_unique_bucket(refs, band_index, bucket_key);
    };

    add_exact_buckets(kFingerprintBandCount, prefix_bucket_key(fingerprint));
    for (std::size_t band_index = 0; band_index < kFingerprintBandCount; ++band_index) {
        add_exact_buckets(static_cast<std::uint32_t>(band_index),
                          band_bucket_key(fingerprint, band_index));
    }

    if (for_nearest) {
        return refs;
    }

    const std::uint32_t radius = neighbor_radius(config);
    const std::vector<BandBucketRef> exact_refs = refs;
    for (const BandBucketRef& exact_ref : exact_refs) {
        for (std::uint32_t delta = 1; delta <= radius; ++delta) {
            if (exact_ref.bucket_key >= delta) {
                append_unique_bucket(refs, exact_ref.band_index, exact_ref.bucket_key - delta);
            }
            if (exact_ref.bucket_key + delta < kBucketSpace) {
                append_unique_bucket(refs, exact_ref.band_index, exact_ref.bucket_key + delta);
            }
        }
    }
    return refs;
}

std::uint64_t bucket_lookup_key(std::uint32_t band_index, std::uint32_t bucket_key) {
    return (static_cast<std::uint64_t>(band_index) << 32) |
           static_cast<std::uint64_t>(bucket_key);
}

} // namespace biopic
