#include "test_images.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <iomanip>
#include <sstream>

namespace biopic::test_support {

SyntheticImage make_uniform_rgb(int width, int height, std::uint8_t r, std::uint8_t g,
                                std::uint8_t b) {
    SyntheticImage image{.width = width, .height = height};
    image.rgb.resize(static_cast<std::size_t>(width * height * 3));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t index = static_cast<std::size_t>((y * width + x) * 3);
            image.rgb[index] = r;
            image.rgb[index + 1] = g;
            image.rgb[index + 2] = b;
        }
    }
    return image;
}

SyntheticImage make_gradient_rgb(int width, int height) {
    SyntheticImage image{.width = width, .height = height};
    image.rgb.resize(static_cast<std::size_t>(width * height * 3));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t index = static_cast<std::size_t>((y * width + x) * 3);
            image.rgb[index] =
                static_cast<std::uint8_t>((x * 255) / std::max(1, width - 1));
            image.rgb[index + 1] =
                static_cast<std::uint8_t>((y * 255) / std::max(1, height - 1));
            image.rgb[index + 2] =
                static_cast<std::uint8_t>(((x + y) * 255) / std::max(1, width + height - 2));
        }
    }
    return image;
}

SyntheticImage make_checkerboard_rgb(int width, int height, int cell) {
    SyntheticImage image{.width = width, .height = height};
    image.rgb.resize(static_cast<std::size_t>(width * height * 3));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const bool dark = ((x / cell) + (y / cell)) % 2 == 0;
            const std::uint8_t value = dark ? 32 : 224;
            const std::size_t index = static_cast<std::size_t>((y * width + x) * 3);
            image.rgb[index] = value;
            image.rgb[index + 1] = value;
            image.rgb[index + 2] = value;
        }
    }
    return image;
}

namespace {

cv::Mat to_bgr_mat(const SyntheticImage& image) {
    cv::Mat rgb(image.height, image.width, CV_8UC3, const_cast<std::uint8_t*>(image.rgb.data()));
    cv::Mat bgr;
    cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
    return bgr;
}

} // namespace

std::vector<std::uint8_t> encode_png(const SyntheticImage& image) {
    std::vector<std::uint8_t> encoded;
    cv::imencode(".png", to_bgr_mat(image), encoded);
    return encoded;
}

std::vector<std::uint8_t> encode_jpeg(const SyntheticImage& image, int quality) {
    std::vector<std::uint8_t> encoded;
    const std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, quality};
    cv::imencode(".jpg", to_bgr_mat(image), encoded, params);
    return encoded;
}

std::string fingerprint_to_hex(const std::array<std::uint8_t, 144>& bytes) {
    std::ostringstream oss;
    for (const auto byte : bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

} // namespace biopic::test_support
