#include "cli_common.hpp"

#include "biopic/env.hpp"
#include "biopic/index/hash_match_config.hpp"

#include <iostream>

namespace biopic::cli {

ParsedArgs parse_args(int argc, char** argv, int positional_start) {
    ParsedArgs args;
    for (int index = positional_start; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            args.help = true;
            continue;
        }
        if (argument == "--json") {
            args.json_output = true;
            continue;
        }
        if (argument == "--config" && index + 1 < argc) {
            args.config = std::filesystem::path(argv[++index]);
            continue;
        }
        if (argument == "--database" && index + 1 < argc) {
            args.database = std::filesystem::path(argv[++index]);
            continue;
        }
        if (argument == "--label" && index + 1 < argc) {
            args.label = std::string(argv[++index]);
            continue;
        }
        if (argument == "--threshold" && index + 1 < argc) {
            const std::string value = argv[++index];
            double parsed = 0.0;
            const auto* begin = value.data();
            const auto* end = begin + value.size();
            const auto result = std::from_chars(begin, end, parsed);
            if (result.ec == std::errc() && result.ptr == end) {
                args.threshold = parsed;
            }
            continue;
        }
        if (argument == "--metric" && index + 1 < argc) {
            args.metric = parse_distance_metric(argv[++index]);
            continue;
        }
        if (!argument.empty() && argument[0] == '-') {
            continue;
        }
        args.positional.push_back(argument);
    }
    return args;
}

HashMatchConfig resolve_match_config(const ParsedArgs& args) {
    return resolve_hash_match_config(args.threshold, args.metric);
}

std::optional<std::filesystem::path> resolve_classifier_config(const ParsedArgs& args) {
    if (args.config.has_value()) {
        return args.config;
    }
    const auto config_from_env = read_env_variable("BIOPIC_CLASSIFIER_CONFIG");
    if (config_from_env.has_value()) {
        return std::filesystem::path(*config_from_env);
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> resolve_database_path(const ParsedArgs& args) {
    if (args.database.has_value()) {
        return args.database;
    }
    const auto database_from_env = read_env_variable("BIOPIC_DATABASE");
    if (database_from_env.has_value()) {
        return std::filesystem::path(*database_from_env);
    }
    return std::nullopt;
}

void print_root_usage() {
    std::cerr
        << "BioPic — open-source image moderation engine\n\n"
        << "Usage:\n"
        << "  biopic <command> [arguments]\n\n"
        << "Commands:\n"
        << "  hash IMAGE\n"
        << "  compare IMAGE_A IMAGE_B\n"
        << "  scan IMAGE [--database PATH] [--config PATH] [--json]\n"
        << "  evaluate DATASET [--database PATH] [--config PATH]\n"
        << "  benchmark similarity [--records N]\n"
        << "  database add IMAGE --label LABEL [--database PATH]\n"
        << "  database search IMAGE [--database PATH]\n"
        << "  database stats [--database PATH]\n"
        << "  database vacuum [--database PATH]\n"
        << "  model info [--config PATH]\n"
        << "  model verify [--config PATH]\n"
        << "  model list\n"
        << "  config\n"
        << "  doctor [--database PATH] [--config PATH]\n"
        << "  version\n\n"
        << "Environment:\n"
        << "  BIOPIC_CLASSIFIER_CONFIG  Default classifier config path\n"
        << "  BIOPIC_DATABASE         Default fingerprint database path\n\n"
        << "Exit codes (scan): 0=ALLOW, 1=REVIEW, 2=BLOCK, 10=error\n";
}

void print_command_usage(std::string_view command) {
    print_root_usage();
    std::cerr << "Command help requested: " << command << '\n';
}

} // namespace biopic::cli
