#include "biopic/ai/preprocessing.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>

namespace biopic {

namespace {

cv::Mat image_view_to_bgr(const ImageView& image) {
    cv::Mat bgr(image.height(), image.width(), CV_8UC3);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const std::uint8_t red = image.red_at(x, y);
            const std::uint8_t green = image.green_at(x, y);
            const std::uint8_t blue = image.blue_at(x, y);
            bgr.at<cv::Vec3b>(y, x) = cv::Vec3b(blue, green, red);
        }
    }
    return bgr;
}

} // namespace

Preprocessor::Preprocessor(ModelInputRequirements requirements)
    : requirements_(requirements) {}

std::optional<PreparedTensor> Preprocessor::prepare(const ImageView& image) const {
    if (image.empty()) {
        return std::nullopt;
    }
    if (requirements_.width <= 0 || requirements_.height <= 0 || requirements_.channels <= 0) {
        return std::nullopt;
    }

    const cv::Mat source = image_view_to_bgr(image);
    cv::Mat resized;
    cv::resize(source, resized,
               cv::Size(requirements_.width, requirements_.height), 0.0, 0.0, cv::INTER_AREA);

    PreparedTensor tensor;
    tensor.width = requirements_.width;
    tensor.height = requirements_.height;
    tensor.channels = requirements_.channels;
    tensor.data.resize(static_cast<std::size_t>(tensor.width) * static_cast<std::size_t>(tensor.height) *
                       static_cast<std::size_t>(tensor.channels));

    const int channel_count = std::min(requirements_.channels, resized.channels());
    const std::size_t plane_size =
        static_cast<std::size_t>(tensor.width) * static_cast<std::size_t>(tensor.height);

    for (int channel = 0; channel < channel_count; ++channel) {
        const float mean = requirements_.mean[static_cast<std::size_t>(channel)];
        const float stddev = requirements_.stddev[static_cast<std::size_t>(channel)];
        const float inverse_stddev = stddev == 0.0f ? 0.0f : 1.0f / stddev;

        for (int y = 0; y < tensor.height; ++y) {
            for (int x = 0; x < tensor.width; ++x) {
                const cv::Vec3b pixel_bgr = resized.at<cv::Vec3b>(y, x);
                const float channels_rgb[3] = {
                    static_cast<float>(pixel_bgr[2]),
                    static_cast<float>(pixel_bgr[1]),
                    static_cast<float>(pixel_bgr[0]),
                };
                const float pixel = channels_rgb[channel] * static_cast<float>(requirements_.scale);
                const float normalized = (pixel - mean) * inverse_stddev;
                tensor.data[channel * plane_size + static_cast<std::size_t>(y) *
                                                        static_cast<std::size_t>(tensor.width) +
                                    static_cast<std::size_t>(x)] = normalized;
            }
        }
    }

    return tensor;
}

} // namespace biopic
