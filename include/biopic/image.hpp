#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "biopic/types.hpp"

namespace biopic {

class ImageView {
  public:
    ImageView() = default;
    ImageView(int width, int height, std::span<const std::uint8_t> rgb);

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] std::span<const std::uint8_t> data() const noexcept { return data_; }
    [[nodiscard]] bool empty() const noexcept { return width_ <= 0 || height_ <= 0 || data_.empty(); }

    [[nodiscard]] std::uint8_t red_at(int x, int y) const;
    [[nodiscard]] std::uint8_t green_at(int x, int y) const;
    [[nodiscard]] std::uint8_t blue_at(int x, int y) const;

  private:
    int width_ = 0;
    int height_ = 0;
    std::span<const std::uint8_t> data_;
};

struct DecodedImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgb;
    std::string sha256_hex;
};

struct DecodeError {
    std::string message;
};

class ImageDecoder {
  public:
    explicit ImageDecoder(DecodeLimits limits = {});

    [[nodiscard]] std::optional<DecodedImage> decode(std::span<const std::uint8_t> encoded) const;

    [[nodiscard]] std::optional<DecodedImage> decode_file(const std::string& path) const;

    [[nodiscard]] const DecodeLimits& limits() const noexcept { return limits_; }

  private:
    DecodeLimits limits_;
};

// SHA-256 over canonical 8-bit RGB row-major bytes (width * height * 3).
[[nodiscard]] std::string sha256_rgb(const ImageView& image);

} // namespace biopic
