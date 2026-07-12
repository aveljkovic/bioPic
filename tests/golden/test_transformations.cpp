#include <gtest/gtest.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "biopic/distance.hpp"
#include "biopic/hasher.hpp"
#include "biopic/image.hpp"

#include "test_images.hpp"

namespace {

biopic::Fingerprint hash_bgr_mat(const cv::Mat& bgr) {
    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(rgb.total() * rgb.channels()));
    std::copy_n(rgb.ptr<std::uint8_t>(), buffer.size(), buffer.data());
    biopic::ImageView view(rgb.cols, rgb.rows, buffer);
    biopic::Hasher hasher;
    return hasher.compute(view);
}

cv::Mat synthetic_to_bgr(const biopic::test_support::SyntheticImage& image) {
    cv::Mat rgb(image.height, image.width, CV_8UC3,
                const_cast<std::uint8_t*>(image.rgb.data()));
    cv::Mat bgr;
    cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
    return bgr;
}

} // namespace

TEST(TransformationDistanceTest, MeasuresBenignTransformationsWithoutAssumingExactMatch) {
    const auto source_image = biopic::test_support::make_gradient_rgb(160, 120);
    const cv::Mat baseline_bgr = synthetic_to_bgr(source_image);
    const auto baseline = hash_bgr_mat(baseline_bgr);

    const auto distance_for = [&](const cv::Mat& transformed) {
        const auto fingerprint = hash_bgr_mat(transformed);
        return biopic::compare_fingerprints(baseline, fingerprint, biopic::DistanceMetric::L2)
            .value;
    };

    cv::Mat resized;
    cv::resize(baseline_bgr, resized, cv::Size(), 0.75, 0.75, cv::INTER_AREA);
    const double resize_distance = distance_for(resized);
    EXPECT_GT(resize_distance, 0.0);
    EXPECT_LT(resize_distance, 80.0);

    std::vector<std::uint8_t> jpeg_buffer;
    cv::imencode(".jpg", baseline_bgr, jpeg_buffer, {cv::IMWRITE_JPEG_QUALITY, 70});
    const cv::Mat recompressed = cv::imdecode(jpeg_buffer, cv::IMREAD_COLOR);
    const double jpeg_distance = distance_for(recompressed);
    EXPECT_GT(jpeg_distance, 0.0);
    EXPECT_LT(jpeg_distance, 60.0);

    cv::Mat brighter;
    baseline_bgr.convertTo(brighter, -1, 1.0, 12.0);
    const double brightness_distance = distance_for(brighter);
    EXPECT_GT(brightness_distance, 0.0);
    EXPECT_LT(brightness_distance, 50.0);

    cv::Mat higher_contrast;
    baseline_bgr.convertTo(higher_contrast, -1, 1.08, 0.0);
    const double contrast_distance = distance_for(higher_contrast);
    EXPECT_GT(contrast_distance, 0.0);
    EXPECT_LT(contrast_distance, 50.0);

    cv::Mat blurred;
    cv::GaussianBlur(baseline_bgr, blurred, cv::Size(3, 3), 0.6);
    const double blur_distance = distance_for(blurred);
    EXPECT_LT(blur_distance, 70.0);

    cv::Mat translated;
    cv::Mat translation_matrix = (cv::Mat_<double>(2, 3) << 1, 0, 2, 0, 1, 1);
    cv::warpAffine(baseline_bgr, translated, translation_matrix, baseline_bgr.size(),
                   cv::INTER_LINEAR, cv::BORDER_REPLICATE);
    const double translation_distance = distance_for(translated);
    EXPECT_GT(translation_distance, 0.0);
    EXPECT_LT(translation_distance, 80.0);
}
