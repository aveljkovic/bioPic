#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "biopic/types.hpp"

namespace biopic {

using FingerprintBytes = std::array<std::uint8_t, kFingerprintComponentCount>;

struct Fingerprint {
    HashAlgorithm algorithm = HashAlgorithm::BioPic;
    std::uint32_t version = kHashAlgorithmVersion;
    FingerprintBytes bytes{};

    [[nodiscard]] bool is_zero() const noexcept;
};

enum class FingerprintEncoding { Raw, Hex, Base64 };

[[nodiscard]] std::string encode_fingerprint(const Fingerprint& fingerprint,
                                             FingerprintEncoding encoding);

[[nodiscard]] std::optional<Fingerprint>
decode_fingerprint(std::string_view encoded, FingerprintEncoding encoding,
                   std::uint32_t expected_version = kHashAlgorithmVersion);

} // namespace biopic
