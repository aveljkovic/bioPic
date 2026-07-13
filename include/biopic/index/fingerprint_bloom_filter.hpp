#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "biopic/fingerprint.hpp"

namespace biopic {

class FingerprintBloomFilter {
  public:
    void reset(std::size_t expected_entries, double false_positive_rate = 0.01);

    void add(const Fingerprint& fingerprint);
    [[nodiscard]] bool might_contain(const Fingerprint& fingerprint) const noexcept;

  private:
    [[nodiscard]] std::uint64_t hash_slot(const Fingerprint& fingerprint,
                                          std::uint32_t seed) const noexcept;

    std::vector<std::uint64_t> bits_;
    std::size_t bit_count_ = 0;
    std::uint32_t hash_count_ = 0;
};

} // namespace biopic
