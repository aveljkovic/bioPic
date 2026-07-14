#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "biopic/types.hpp"

namespace biopic::evaluation {

struct DatasetSample {
    std::filesystem::path image_path;
    ModerationDecision expected = ModerationDecision::Allow;
};

[[nodiscard]] std::optional<ModerationDecision> parse_expected_decision(std::string_view text);

[[nodiscard]] std::vector<DatasetSample> load_manifest(const std::filesystem::path& dataset_root);

} // namespace biopic::evaluation
