#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace biopic::test_support {

struct SyntheticImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgb;
};

SyntheticImage make_uniform_rgb(int width, int height, std::uint8_t r, std::uint8_t g, std::uint8_t b);
SyntheticImage make_gradient_rgb(int width, int height);
SyntheticImage make_checkerboard_rgb(int width, int height, int cell = 8);

std::vector<std::uint8_t> encode_png(const SyntheticImage& image);
std::vector<std::uint8_t> encode_jpeg(const SyntheticImage& image, int quality = 90);

std::string fingerprint_to_hex(const std::array<std::uint8_t, 144>& bytes);

} // namespace biopic::test_support
