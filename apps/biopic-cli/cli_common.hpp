#pragma once

#include <charconv>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "biopic/index/hash_match_config.hpp"
#include "biopic/types.hpp"

namespace biopic::cli {

struct ParsedArgs {
    std::vector<std::string> positional;
    std::optional<std::filesystem::path> config;
    std::optional<std::filesystem::path> database;
    std::optional<double> threshold;
    std::optional<DistanceMetric> metric;
    std::optional<std::string> label;
    bool json_output = false;
    bool help = false;
};

[[nodiscard]] ParsedArgs parse_args(int argc, char** argv, int positional_start = 2);

[[nodiscard]] HashMatchConfig resolve_match_config(const ParsedArgs& args);

[[nodiscard]] std::optional<std::filesystem::path> resolve_classifier_config(const ParsedArgs& args);

[[nodiscard]] std::optional<std::filesystem::path> resolve_database_path(const ParsedArgs& args);

void print_root_usage();

void print_command_usage(std::string_view command);

} // namespace biopic::cli
