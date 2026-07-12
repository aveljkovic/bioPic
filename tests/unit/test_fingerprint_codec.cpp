#include <gtest/gtest.h>

#include "biopic/fingerprint.hpp"

TEST(FingerprintCodecTest, HexRoundTrip) {
    biopic::Fingerprint fingerprint;
    for (std::size_t i = 0; i < fingerprint.bytes.size(); ++i) {
        fingerprint.bytes[i] = static_cast<std::uint8_t>(i);
    }

    const std::string encoded =
        biopic::encode_fingerprint(fingerprint, biopic::FingerprintEncoding::Hex);
    const auto decoded = biopic::decode_fingerprint(encoded, biopic::FingerprintEncoding::Hex);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->bytes, fingerprint.bytes);
}

TEST(FingerprintCodecTest, Base64RoundTrip) {
    biopic::Fingerprint fingerprint;
    fingerprint.bytes.fill(127);

    const std::string encoded =
        biopic::encode_fingerprint(fingerprint, biopic::FingerprintEncoding::Base64);
    const auto decoded = biopic::decode_fingerprint(encoded, biopic::FingerprintEncoding::Base64);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->bytes, fingerprint.bytes);
}

TEST(FingerprintCodecTest, RejectsInvalidHex) {
    const auto decoded = biopic::decode_fingerprint("biopic:v1:nothex", biopic::FingerprintEncoding::Hex);
    EXPECT_FALSE(decoded.has_value());
}
