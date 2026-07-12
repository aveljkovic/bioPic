#include "biopic/hasher.hpp"

#include "biopic/hash_parameters.hpp"
#include "biopic/image.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace biopic {

namespace {

using IntegralImage = std::vector<std::int64_t>;

double clamp_double(double value, double low, double high) {
    return std::max(low, std::min(high, value));
}

double l2_norm(const std::array<double, HashParameters::component_count>& values) {
    double sum = 0.0;
    for (const double value : values) {
        sum += value * value;
    }
    return std::sqrt(sum);
}

void normalize_and_clip(std::array<double, HashParameters::component_count>& values) {
    const double norm = l2_norm(values);
    if (norm < HashParameters::epsilon) {
        values.fill(0.0);
        return;
    }

    for (double& value : values) {
        value /= norm;
    }
    for (double& value : values) {
        value = std::min(value, HashParameters::clip_value);
    }

    const double norm2 = l2_norm(values);
    if (norm2 < HashParameters::epsilon) {
        values.fill(0.0);
        return;
    }
    for (double& value : values) {
        value /= norm2;
    }
}

std::uint8_t quantize_component(double value) {
    const double normalized = clamp_double(value / HashParameters::clip_value, 0.0, 1.0);
    const double scaled = normalized * 255.0;
    return static_cast<std::uint8_t>(std::llround(scaled));
}

bool is_spatially_uniform_rgb(const ImageView& image) {
    if (image.empty()) {
        return true;
    }

    const std::uint8_t reference_red = image.red_at(0, 0);
    const std::uint8_t reference_green = image.green_at(0, 0);
    const std::uint8_t reference_blue = image.blue_at(0, 0);

    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.red_at(x, y) != reference_red || image.green_at(x, y) != reference_green ||
                image.blue_at(x, y) != reference_blue) {
                return false;
            }
        }
    }
    return true;
}

IntegralImage build_integral_image(const ImageView& image) {
    const int width = image.width();
    const int height = image.height();
    IntegralImage integral(static_cast<std::size_t>((width + 1) * (height + 1)), 0);

    auto index = [&](int x, int y) -> std::size_t {
        return static_cast<std::size_t>(y * (width + 1) + x);
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::int64_t intensity = static_cast<std::int64_t>(image.red_at(x, y)) +
                                             static_cast<std::int64_t>(image.green_at(x, y)) +
                                             static_cast<std::int64_t>(image.blue_at(x, y));
            integral[index(x + 1, y + 1)] =
                intensity + integral[index(x, y + 1)] + integral[index(x + 1, y)] -
                integral[index(x, y)];
        }
    }
    return integral;
}

double interpolate_integral(const IntegralImage& integral, int stride, double x, double y) {
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = x0 + 1;
    const int y1 = y0 + 1;

    const double tx = x - static_cast<double>(x0);
    const double ty = y - static_cast<double>(y0);

    const auto at = [&](int ix, int iy) -> double {
        ix = std::max(0, std::min(ix, stride - 1));
        iy = std::max(0, std::min(iy, static_cast<int>(integral.size() / static_cast<std::size_t>(stride)) - 1));
        return static_cast<double>(integral[static_cast<std::size_t>(iy * stride + ix)]);
    };

    const double v00 = at(x0, y0);
    const double v10 = at(x1, y0);
    const double v01 = at(x0, y1);
    const double v11 = at(x1, y1);

    const double top = v00 * (1.0 - tx) + v10 * tx;
    const double bottom = v01 * (1.0 - tx) + v11 * tx;
    return top * (1.0 - ty) + bottom * ty;
}

double rectangle_mean(const IntegralImage& integral, int integral_stride, int image_width,
                      int image_height, double left, double top, double right, double bottom) {
    const double clipped_left = clamp_double(left, 0.0, static_cast<double>(image_width));
    const double clipped_top = clamp_double(top, 0.0, static_cast<double>(image_height));
    const double clipped_right = clamp_double(right, 0.0, static_cast<double>(image_width));
    const double clipped_bottom = clamp_double(bottom, 0.0, static_cast<double>(image_height));

    const double width = clipped_right - clipped_left;
    const double height = clipped_bottom - clipped_top;
    if (width <= 0.0 || height <= 0.0) {
        return 0.0;
    }

    const double top_left = interpolate_integral(integral, integral_stride, clipped_left, clipped_top);
    const double top_right =
        interpolate_integral(integral, integral_stride, clipped_right, clipped_top);
    const double bottom_left =
        interpolate_integral(integral, integral_stride, clipped_left, clipped_bottom);
    const double bottom_right =
        interpolate_integral(integral, integral_stride, clipped_right, clipped_bottom);

    const double sum = bottom_right - bottom_left - top_right + top_left;
    return sum / (width * height);
}

void splat_bilinear(std::array<double, HashParameters::component_count>& output, std::size_t channel,
                    double x, double y, double value) {
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = x0 + 1;
    const int y1 = y0 + 1;

    const double tx = x - static_cast<double>(x0);
    const double ty = y - static_cast<double>(y0);

    const auto accumulate = [&](int ox, int oy, double weight) {
        if (ox < 0 || oy < 0 ||
            ox >= static_cast<int>(HashParameters::output_grid_width) ||
            oy >= static_cast<int>(HashParameters::output_grid_height) || weight <= 0.0) {
            return;
        }
        const std::size_t index = channel * HashParameters::output_grid_width *
                                      HashParameters::output_grid_height +
                                  static_cast<std::size_t>(oy) * HashParameters::output_grid_width +
                                  static_cast<std::size_t>(ox);
        output[index] += value * weight;
    };

    accumulate(x0, y0, (1.0 - tx) * (1.0 - ty));
    accumulate(x1, y0, tx * (1.0 - ty));
    accumulate(x0, y1, (1.0 - tx) * ty);
    accumulate(x1, y1, tx * ty);
}

Fingerprint compute_v1(const ImageView& image) {
    Fingerprint fingerprint;
    fingerprint.algorithm = HashAlgorithm::BioPic;
    fingerprint.version = HashParameters::version;

    if (image.empty() || is_spatially_uniform_rgb(image)) {
        return fingerprint;
    }

    const int width = image.width();
    const int height = image.height();
    const double min_dim = static_cast<double>(std::min(width, height));
    const IntegralImage integral = build_integral_image(image);
    const int integral_stride = width + 1;

    std::array<double, HashParameters::feature_grid_width * HashParameters::feature_grid_height>
        feature_map{};
    double feature_peak = 0.0;
    for (std::size_t j = 0; j < HashParameters::feature_grid_height; ++j) {
        for (std::size_t i = 0; i < HashParameters::feature_grid_width; ++i) {
            const double nu =
                (static_cast<double>(i) + 0.5) /
                static_cast<double>(HashParameters::feature_grid_width);
            const double nv =
                (static_cast<double>(j) + 0.5) /
                static_cast<double>(HashParameters::feature_grid_height);
            const double cx = nu * static_cast<double>(width);
            const double cy = nv * static_cast<double>(height);

            double weighted_sum = 0.0;
            for (std::size_t scale = 0; scale < HashParameters::box_radii_relative.size(); ++scale) {
                const double radius =
                    HashParameters::box_radii_relative[scale] * min_dim;
                const double mean = rectangle_mean(integral, integral_stride, width, height,
                                                   cx - radius, cy - radius, cx + radius,
                                                   cy + radius);
                weighted_sum += HashParameters::scale_weights[scale] * mean;
            }
            feature_map[j * HashParameters::feature_grid_width + i] = weighted_sum;
            feature_peak = std::max(feature_peak, std::abs(weighted_sum));
        }
    }

    std::array<double, HashParameters::component_count> splat_grid{};
    const double feature_to_output_scale =
        static_cast<double>(HashParameters::output_grid_width - 1) /
        static_cast<double>(HashParameters::feature_grid_width - 1);

    for (std::size_t j = 1; j + 1 < HashParameters::feature_grid_height; ++j) {
        for (std::size_t i = 1; i + 1 < HashParameters::feature_grid_width; ++i) {
            const std::size_t center = j * HashParameters::feature_grid_width + i;
            const double gx =
                feature_map[center + 1] - feature_map[center - 1];
            const double gy =
                feature_map[center + HashParameters::feature_grid_width] -
                feature_map[center - HashParameters::feature_grid_width];

            const double channels[4] = {std::max(gx, 0.0), std::max(-gx, 0.0), std::max(gy, 0.0),
                                        std::max(-gy, 0.0)};

            const double out_x = static_cast<double>(i) * feature_to_output_scale;
            const double out_y = static_cast<double>(j) * feature_to_output_scale;

            for (std::size_t channel = 0; channel < HashParameters::directional_channels; ++channel) {
                splat_bilinear(splat_grid, channel, out_x, out_y, channels[channel]);
            }
        }
    }

    const double raw_gradient_norm = l2_norm(splat_grid);
    const double low_energy_threshold =
        HashParameters::relative_low_energy_ratio * feature_peak;
    if (raw_gradient_norm <= low_energy_threshold || raw_gradient_norm < HashParameters::epsilon) {
        return fingerprint;
    }

    normalize_and_clip(splat_grid);

    for (std::size_t i = 0; i < fingerprint.bytes.size(); ++i) {
        fingerprint.bytes[i] = quantize_component(splat_grid[i]);
    }
    return fingerprint;
}

} // namespace

Hasher::Hasher(std::uint32_t version) : version_(version) {}

Fingerprint Hasher::compute(const ImageView& image) const {
    if (version_ == HashParameters::version) {
        return compute_v1(image);
    }
    return {};
}

} // namespace biopic
