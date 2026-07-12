#pragma once

#include <array>
#include <cstddef>

#include "biopic/types.hpp"

namespace biopic {

// Immutable version-1 parameters. Any change that alters output requires a new hash version.
struct HashParameters {
    static constexpr std::uint32_t version = 1;

    static constexpr std::size_t feature_grid_width = 12;
    static constexpr std::size_t feature_grid_height = 12;
    static constexpr std::size_t output_grid_width = 6;
    static constexpr std::size_t output_grid_height = 6;
    static constexpr std::size_t directional_channels = 4;
    static constexpr std::size_t component_count = output_grid_width * output_grid_height *
                                                   directional_channels;

    static constexpr std::array<double, 3> box_radii_relative = {0.02, 0.05, 0.10};
    static constexpr std::array<double, 3> scale_weights = {0.50, 0.30, 0.20};

    static constexpr double clip_value = 0.20;
    static constexpr double epsilon = 1e-12;

    // Degenerate-gradient noise floor: sqrt(component_count) * machine_epsilon * 8, scaled by
    // peak feature-map magnitude before L2 normalization. Suppresses cross-platform FP residue
    // without affecting images that carry genuine gradient energy.
    static constexpr double relative_low_energy_ratio =
        12.0 * 2.2204460492503131e-16 * 8.0;
};

static_assert(HashParameters::component_count == kFingerprintComponentCount);

} // namespace biopic
