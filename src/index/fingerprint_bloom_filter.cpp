#include "biopic/index/fingerprint_bloom_filter.hpp"

#include <cmath>

namespace biopic {

namespace {

constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

std::uint64_t fnv1a64(std::uint64_t hash, std::uint8_t byte) {
    hash ^= static_cast<std::uint64_t>(byte);
    return hash * kFnvPrime;
}

} // namespace

void FingerprintBloomFilter::reset(std::size_t expected_entries, double false_positive_rate) {
    if (expected_entries == 0) {
        bits_.clear();
        bit_count_ = 0;
        hash_count_ = 0;
        return;
    }

    const double ln2 = std::log(2.0);
    const double numerator = -static_cast<double>(expected_entries) * std::log(false_positive_rate);
    const double denominator = ln2 * ln2;
    bit_count_ = static_cast<std::size_t>(std::ceil(numerator / denominator));
    if (bit_count_ < 64) {
        bit_count_ = 64;
    }

    hash_count_ = static_cast<std::uint32_t>(
        std::max(1.0, std::round((static_cast<double>(bit_count_) / expected_entries) * ln2)));
    const std::size_t word_count = (bit_count_ + 63) / 64;
    bits_.assign(word_count, 0ULL);
}

void FingerprintBloomFilter::add(const Fingerprint& fingerprint) {
    if (bit_count_ == 0) {
        return;
    }

    for (std::uint32_t hash_index = 0; hash_index < hash_count_; ++hash_index) {
        const std::size_t slot = hash_slot(fingerprint, hash_index) % bit_count_;
        const std::size_t word_index = slot / 64;
        const std::size_t bit_index = slot % 64;
        bits_[word_index] |= (1ULL << bit_index);
    }
}

bool FingerprintBloomFilter::might_contain(const Fingerprint& fingerprint) const noexcept {
    if (bit_count_ == 0) {
        return false;
    }

    for (std::uint32_t hash_index = 0; hash_index < hash_count_; ++hash_index) {
        const std::size_t slot = hash_slot(fingerprint, hash_index) % bit_count_;
        const std::size_t word_index = slot / 64;
        const std::size_t bit_index = slot % 64;
        if ((bits_[word_index] & (1ULL << bit_index)) == 0ULL) {
            return false;
        }
    }
    return true;
}

std::uint64_t FingerprintBloomFilter::hash_slot(const Fingerprint& fingerprint,
                                                std::uint32_t seed) const noexcept {
    std::uint64_t hash = kFnvOffsetBasis ^ static_cast<std::uint64_t>(seed);
    hash = fnv1a64(hash, static_cast<std::uint8_t>(fingerprint.algorithm));
    hash = fnv1a64(hash, static_cast<std::uint8_t>(fingerprint.version));
    for (std::uint8_t byte : fingerprint.bytes) {
        hash = fnv1a64(hash, byte);
    }
    return hash;
}

} // namespace biopic
