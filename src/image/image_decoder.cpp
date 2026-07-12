#include "biopic/image.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace biopic {

namespace {

std::string bytes_to_hex(std::span<const std::uint8_t> data) {
    std::ostringstream oss;
    for (const auto byte : data) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

std::array<std::uint8_t, 32> sha256_bytes(std::span<const std::uint8_t> data) {
    // OpenCV does not ship SHA-256; use a compact public-domain implementation.
    static constexpr std::array<std::uint32_t, 64> kRoundConstants = {
        0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U,
        0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
        0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U,
        0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
        0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
        0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
        0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
        0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
        0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU,
        0x5b9cca4fU, 0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
        0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

    auto rotr = [](std::uint32_t value, int bits) {
        return (value >> bits) | (value << (32 - bits));
    };

    std::uint64_t bit_length = static_cast<std::uint64_t>(data.size()) * 8U;
    std::vector<std::uint8_t> message(data.begin(), data.end());
    message.push_back(0x80);
    while ((message.size() % 64) != 56) {
        message.push_back(0x00);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        message.push_back(static_cast<std::uint8_t>((bit_length >> shift) & 0xffU));
    }

    std::uint32_t h0 = 0x6a09e667U;
    std::uint32_t h1 = 0xbb67ae85U;
    std::uint32_t h2 = 0x3c6ef372U;
    std::uint32_t h3 = 0xa54ff53aU;
    std::uint32_t h4 = 0x510e527fU;
    std::uint32_t h5 = 0x9b05688cU;
    std::uint32_t h6 = 0x1f83d9abU;
    std::uint32_t h7 = 0x5be0cd19U;

    for (std::size_t chunk = 0; chunk < message.size(); chunk += 64) {
        std::array<std::uint32_t, 64> w{};
        for (std::size_t i = 0; i < 16; ++i) {
            const std::size_t offset = chunk + i * 4;
            w[i] = (static_cast<std::uint32_t>(message[offset]) << 24) |
                   (static_cast<std::uint32_t>(message[offset + 1]) << 16) |
                   (static_cast<std::uint32_t>(message[offset + 2]) << 8) |
                   static_cast<std::uint32_t>(message[offset + 3]);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        std::uint32_t a = h0;
        std::uint32_t b = h1;
        std::uint32_t c = h2;
        std::uint32_t d = h3;
        std::uint32_t e = h4;
        std::uint32_t f = h5;
        std::uint32_t g = h6;
        std::uint32_t h = h7;

        for (std::size_t i = 0; i < 64; ++i) {
            const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const std::uint32_t ch = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + s1 + ch + kRoundConstants[i] + w[i];
            const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
        h5 += f;
        h6 += g;
        h7 += h;
    }

    std::array<std::uint8_t, 32> digest{};
    const std::array<std::uint32_t, 8> state = {h0, h1, h2, h3, h4, h5, h6, h7};
    for (std::size_t i = 0; i < state.size(); ++i) {
        digest[i * 4] = static_cast<std::uint8_t>((state[i] >> 24) & 0xffU);
        digest[i * 4 + 1] = static_cast<std::uint8_t>((state[i] >> 16) & 0xffU);
        digest[i * 4 + 2] = static_cast<std::uint8_t>((state[i] >> 8) & 0xffU);
        digest[i * 4 + 3] = static_cast<std::uint8_t>(state[i] & 0xffU);
    }
    return digest;
}

} // namespace

ImageView::ImageView(int width, int height, std::span<const std::uint8_t> rgb)
    : width_(width), height_(height), data_(rgb) {}

std::uint8_t ImageView::red_at(int x, int y) const {
    const std::size_t index = static_cast<std::size_t>(y * width_ + x) * 3U;
    return data_[index];
}

std::uint8_t ImageView::green_at(int x, int y) const {
    const std::size_t index = static_cast<std::size_t>(y * width_ + x) * 3U + 1U;
    return data_[index];
}

std::uint8_t ImageView::blue_at(int x, int y) const {
    const std::size_t index = static_cast<std::size_t>(y * width_ + x) * 3U + 2U;
    return data_[index];
}

ImageDecoder::ImageDecoder(DecodeLimits limits) : limits_(limits) {}

std::optional<DecodedImage> ImageDecoder::decode(std::span<const std::uint8_t> encoded) const {
    if (encoded.empty()) {
        return std::nullopt;
    }
    if (encoded.size() > limits_.max_encoded_bytes) {
        return std::nullopt;
    }

    cv::Mat buffer(1, static_cast<int>(encoded.size()), CV_8UC1,
                   const_cast<std::uint8_t*>(encoded.data()));
    cv::Mat decoded = cv::imdecode(buffer, cv::IMREAD_COLOR);
    if (decoded.empty()) {
        return std::nullopt;
    }

    if (decoded.cols > limits_.max_width || decoded.rows > limits_.max_height) {
        return std::nullopt;
    }

    const std::uint64_t pixels = static_cast<std::uint64_t>(decoded.cols) *
                                 static_cast<std::uint64_t>(decoded.rows);
    if (pixels > limits_.max_decoded_pixels) {
        return std::nullopt;
    }

    cv::Mat rgb;
    cv::cvtColor(decoded, rgb, cv::COLOR_BGR2RGB);

    DecodedImage result;
    result.width = rgb.cols;
    result.height = rgb.rows;
    result.rgb.resize(static_cast<std::size_t>(rgb.total()) *
                      static_cast<std::size_t>(rgb.channels()));
    if (!rgb.isContinuous()) {
        return std::nullopt;
    }
    std::copy_n(rgb.ptr<std::uint8_t>(), result.rgb.size(), result.rgb.data());

    ImageView view(result.width, result.height, result.rgb);
    result.sha256_hex = sha256_rgb(view);
    return result;
}

std::optional<DecodedImage> ImageDecoder::decode_file(const std::string& path) const {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                    std::istreambuf_iterator<char>());
    return decode(bytes);
}

std::string sha256_rgb(const ImageView& image) {
    if (image.empty()) {
        return bytes_to_hex(sha256_bytes({}));
    }
    return bytes_to_hex(sha256_bytes(image.data()));
}

} // namespace biopic
