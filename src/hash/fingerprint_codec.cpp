#include "biopic/fingerprint.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace biopic {

namespace {

constexpr char kBase64Alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string to_lower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::optional<std::vector<std::uint8_t>> decode_hex(std::string_view encoded) {
    if (encoded.size() % 2 != 0) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> bytes;
    bytes.reserve(encoded.size() / 2);
    for (std::size_t i = 0; i < encoded.size(); i += 2) {
        const auto hex_byte = encoded.substr(i, 2);
        try {
            const auto value = static_cast<std::uint8_t>(std::stoul(std::string(hex_byte), nullptr, 16));
            bytes.push_back(value);
        } catch (...) {
            return std::nullopt;
        }
    }
    return bytes;
}

int base64_value(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A';
    }
    if (ch >= 'a' && ch <= 'z') {
        return ch - 'a' + 26;
    }
    if (ch >= '0' && ch <= '9') {
        return ch - '0' + 52;
    }
    if (ch == '+') {
        return 62;
    }
    if (ch == '/') {
        return 63;
    }
    return -1;
}

std::optional<std::vector<std::uint8_t>> decode_base64(std::string_view encoded) {
    std::vector<std::uint8_t> bytes;
    int val = 0;
    int valb = -8;
    for (const char ch : encoded) {
        if (ch == '=') {
            break;
        }
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            continue;
        }
        const int decoded = base64_value(ch);
        if (decoded < 0) {
            return std::nullopt;
        }
        val = (val << 6) + decoded;
        valb += 6;
        if (valb >= 0) {
            bytes.push_back(static_cast<std::uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return bytes;
}

std::string encode_base64(std::span<const std::uint8_t> data) {
    std::string encoded;
    encoded.reserve(((data.size() + 2) / 3) * 4);
    for (std::size_t i = 0; i < data.size(); i += 3) {
        const std::uint32_t octet_a = data[i];
        const std::uint32_t octet_b = i + 1 < data.size() ? data[i + 1] : 0;
        const std::uint32_t octet_c = i + 2 < data.size() ? data[i + 2] : 0;
        const std::uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        encoded.push_back(kBase64Alphabet[(triple >> 18) & 0x3F]);
        encoded.push_back(kBase64Alphabet[(triple >> 12) & 0x3F]);
        encoded.push_back(i + 1 < data.size() ? kBase64Alphabet[(triple >> 6) & 0x3F] : '=');
        encoded.push_back(i + 2 < data.size() ? kBase64Alphabet[triple & 0x3F] : '=');
    }
    return encoded;
}

} // namespace

std::string encode_fingerprint(const Fingerprint& fingerprint, FingerprintEncoding encoding) {
    std::ostringstream prefix;
    prefix << "biopic:v" << fingerprint.version << ":";

    switch (encoding) {
    case FingerprintEncoding::Raw:
        return prefix.str() + std::string(reinterpret_cast<const char*>(fingerprint.bytes.data()),
                                          fingerprint.bytes.size());
    case FingerprintEncoding::Hex: {
        std::ostringstream hex;
        hex << prefix.str();
        hex << std::hex;
        for (const auto byte : fingerprint.bytes) {
            hex.width(2);
            hex.fill('0');
            hex << static_cast<int>(byte);
        }
        return to_lower(hex.str());
    }
    case FingerprintEncoding::Base64:
        return prefix.str() + encode_base64(fingerprint.bytes);
    }
    return {};
}

std::optional<Fingerprint> decode_fingerprint(std::string_view encoded,
                                              FingerprintEncoding encoding,
                                              std::uint32_t expected_version) {
    Fingerprint fingerprint;
    fingerprint.algorithm = HashAlgorithm::BioPic;

    std::string_view payload = encoded;
    const std::string prefix = "biopic:v";
    if (payload.rfind(prefix, 0) == 0) {
        const auto colon = payload.find(':', prefix.size());
        if (colon == std::string_view::npos) {
            return std::nullopt;
        }
        try {
            fingerprint.version = static_cast<std::uint32_t>(
                std::stoul(std::string(payload.substr(prefix.size(), colon - prefix.size()))));
        } catch (...) {
            return std::nullopt;
        }
        payload = payload.substr(colon + 1);
    } else {
        fingerprint.version = expected_version;
    }

    if (fingerprint.version != expected_version) {
        return std::nullopt;
    }

    switch (encoding) {
    case FingerprintEncoding::Raw:
        if (payload.size() != fingerprint.bytes.size()) {
            return std::nullopt;
        }
        std::copy_n(payload.begin(), fingerprint.bytes.size(), fingerprint.bytes.begin());
        return fingerprint;
    case FingerprintEncoding::Hex: {
        const auto bytes = decode_hex(payload);
        if (!bytes || bytes->size() != fingerprint.bytes.size()) {
            return std::nullopt;
        }
        std::copy(bytes->begin(), bytes->end(), fingerprint.bytes.begin());
        return fingerprint;
    }
    case FingerprintEncoding::Base64: {
        const auto bytes = decode_base64(payload);
        if (!bytes || bytes->size() != fingerprint.bytes.size()) {
            return std::nullopt;
        }
        std::copy(bytes->begin(), bytes->end(), fingerprint.bytes.begin());
        return fingerprint;
    }
    }
    return std::nullopt;
}

} // namespace biopic
