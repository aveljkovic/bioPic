#include "cli_commands.hpp"

#include "cli_common.hpp"

#include "biopic/ai/classifier_config.hpp"
#include "biopic/cli/exit_codes.hpp"
#include "biopic/database/database_maintenance.hpp"
#include "biopic/database/fingerprint_store.hpp"
#include "biopic/distance.hpp"
#include "biopic/env.hpp"
#include "biopic/evaluation/dataset_manifest.hpp"
#include "biopic/evaluation/metrics.hpp"
#include "biopic/fingerprint.hpp"
#include "biopic/hasher.hpp"
#include "biopic/image.hpp"
#include "biopic/index/similarity_index.hpp"
#include "biopic/policy/moderation_policy.hpp"
#include "biopic/version.hpp"
#include "pipeline/scanner.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

namespace biopic::cli {

namespace {

void write_json_string(std::ostream& output, const std::string& value) {
    output << '"';
    for (const char character : value) {
        switch (character) {
        case '\\':
            output << "\\\\";
            break;
        case '"':
            output << "\\\"";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\r':
            output << "\\r";
            break;
        case '\t':
            output << "\\t";
            break;
        default:
            output << character;
            break;
        }
    }
    output << '"';
}

std::unique_ptr<FingerprintStore> open_store(const std::filesystem::path& database_path) {
    auto store = open_persistent_fingerprint_store(database_path);
    if (store == nullptr) {
        std::cerr << "Failed to open database: " << database_path << '\n';
    }
    return store;
}

std::optional<Fingerprint> fingerprint_from_image(const std::filesystem::path& path) {
    ImageDecoder decoder;
    const auto decoded = decoder.decode_file(path.string());
    if (!decoded) {
        return std::nullopt;
    }
    ImageView view(decoded->width, decoded->height, decoded->rgb);
    Hasher hasher;
    return hasher.compute(view);
}

void print_class_metrics(std::string_view name, const evaluation::ClassMetrics& metrics) {
    std::cout << name << ":\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  support:   " << metrics.support << '\n';
    std::cout << "  precision: " << metrics.precision << '\n';
    std::cout << "  recall:    " << metrics.recall << '\n';
    std::cout << "  f1:        " << metrics.f1 << '\n';
}

Fingerprint make_benchmark_fingerprint(std::size_t seed) {
    Fingerprint fingerprint;
    for (std::size_t index = 0; index < fingerprint.bytes.size(); ++index) {
        fingerprint.bytes[index] =
            static_cast<std::uint8_t>((seed + index * 17U + (seed >> 4)) % 256U);
    }
    return fingerprint;
}

} // namespace

int run_version(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const BuildInfo info = build_info();
    std::cout << "BioPic " << info.version_string << '\n';
    std::cout << "Git commit: " << info.git_commit << '\n';
    std::cout << "Build type: " << info.build_type << '\n';
    std::cout << "ONNX Runtime: " << onnx_runtime_version() << '\n';
    std::cout << "SQLite: " << sqlite_version_string() << '\n';
    return 0;
}

int run_config(int argc, char** argv) {
    const ParsedArgs args = parse_args(argc, argv, 2);
    if (args.help) {
        print_command_usage("config");
        return 0;
    }

    const auto config = resolve_classifier_config(args);
    const auto database = resolve_database_path(args);

    std::cout << "BioPic configuration\n\n";
    std::cout << "Classifier config: "
              << (config.has_value() ? config->string() : "(not set)") << '\n';
    std::cout << "Database path: "
              << (database.has_value() ? database->string() : "(not set)") << '\n';
    std::cout << "BIOPIC_CLASSIFIER_CONFIG: "
              << (read_env_variable("BIOPIC_CLASSIFIER_CONFIG").value_or("(not set)")) << '\n';
    std::cout << "BIOPIC_DATABASE: "
              << (read_env_variable("BIOPIC_DATABASE").value_or("(not set)")) << '\n';
    return 0;
}

int run_doctor(int argc, char** argv) {
    const ParsedArgs args = parse_args(argc, argv, 2);
    if (args.help) {
        print_command_usage("doctor");
        return 0;
    }

    bool healthy = true;
    std::cout << "BioPic doctor\n\n";

    const BuildInfo info = build_info();
    std::cout << "[ok] BioPic " << info.version_string << " (" << info.build_type << ")\n";
    std::cout << "[ok] ONNX Runtime " << onnx_runtime_version() << '\n';
    std::cout << "[ok] SQLite " << sqlite_version_string() << '\n';

    Hasher hasher;
    const std::vector<std::uint8_t> rgb(3, 128);
    const ImageView view(1, 1, rgb);
    (void)hasher.compute(view);
    std::cout << "[ok] Fingerprint pipeline available\n";

    const auto config_path = resolve_classifier_config(args);
    if (config_path.has_value()) {
        const auto config = ClassifierConfig::load_from_file(*config_path);
        if (!config.has_value()) {
            std::cout << "[fail] Classifier config invalid: " << *config_path << '\n';
            healthy = false;
        } else if (!std::filesystem::exists(config->model_path)) {
            std::cout << "[fail] Model file missing: " << config->model_path << '\n';
            healthy = false;
        } else {
            const auto classifier = create_classifier(*config);
            if (classifier == nullptr) {
                std::cout << "[fail] Classifier failed to initialize\n";
                healthy = false;
            } else {
                std::cout << "[ok] Classifier config and model loadable\n";
            }
        }
    } else {
        std::cout << "[info] Classifier config not set\n";
    }

    const auto database_path = resolve_database_path(args);
    if (database_path.has_value()) {
        const auto stats = inspect_database(*database_path);
        if (!stats.has_value()) {
            std::cout << "[fail] Database not readable: " << *database_path << '\n';
            healthy = false;
        } else {
            std::cout << "[ok] Database readable (" << stats->fingerprint_count
                      << " fingerprints, " << stats->bucket_count << " buckets)\n";
        }
    } else {
        std::cout << "[info] Database path not set\n";
    }

    return healthy ? 0 : kExitError;
}

int run_hash(int argc, char** argv) {
    const ParsedArgs args = parse_args(argc, argv, 2);
    if (args.help || args.positional.size() != 1) {
        print_command_usage("hash");
        return args.help ? 0 : kExitError;
    }

    const std::filesystem::path path = args.positional.front();
    ImageDecoder decoder;
    const auto decoded = decoder.decode_file(path.string());
    if (!decoded) {
        std::cerr << "Failed to decode image: " << path << '\n';
        return kExitError;
    }

    ImageView view(decoded->width, decoded->height, decoded->rgb);
    Hasher hasher;
    const Fingerprint fingerprint = hasher.compute(view);
    const std::string hex = encode_fingerprint(fingerprint, FingerprintEncoding::Hex);
    const std::string base64 = encode_fingerprint(fingerprint, FingerprintEncoding::Base64);

    if (args.json_output) {
        std::cout << '{';
        std::cout << "\"algorithm\":\"biopic\",";
        std::cout << "\"version\":" << fingerprint.version << ',';
        write_json_string(std::cout, "sha256_rgb");
        std::cout << ':';
        write_json_string(std::cout, decoded->sha256_hex);
        std::cout << ',';
        write_json_string(std::cout, "hex");
        std::cout << ':';
        write_json_string(std::cout, hex);
        std::cout << ',';
        write_json_string(std::cout, "base64");
        std::cout << ':';
        write_json_string(std::cout, base64);
        std::cout << "}\n";
        return 0;
    }

    std::cout << "algorithm: biopic\n";
    std::cout << "version: " << fingerprint.version << '\n';
    std::cout << "sha256_rgb: " << decoded->sha256_hex << '\n';
    std::cout << "hex: " << hex << '\n';
    std::cout << "base64: " << base64 << '\n';
    return 0;
}

int run_compare(int argc, char** argv) {
    const ParsedArgs args = parse_args(argc, argv, 2);
    if (args.help || args.positional.size() != 2) {
        print_command_usage("compare");
        return args.help ? 0 : kExitError;
    }

    ImageDecoder decoder;
    const auto decoded_left = decoder.decode_file(args.positional[0]);
    const auto decoded_right = decoder.decode_file(args.positional[1]);
    if (!decoded_left || !decoded_right) {
        std::cerr << "Failed to decode one or both images\n";
        return kExitError;
    }

    ImageView view_left(decoded_left->width, decoded_left->height, decoded_left->rgb);
    ImageView view_right(decoded_right->width, decoded_right->height, decoded_right->rgb);
    Hasher hasher;
    const Fingerprint left = hasher.compute(view_left);
    const Fingerprint right = hasher.compute(view_right);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "l1: " << compare_fingerprints(left, right, DistanceMetric::L1).value << '\n';
    std::cout << "l2: " << compare_fingerprints(left, right, DistanceMetric::L2).value << '\n';
    std::cout << "l2_squared: "
              << compare_fingerprints(left, right, DistanceMetric::L2Squared).value << '\n';
    return 0;
}

int run_scan(int argc, char** argv) {
    const ParsedArgs args = parse_args(argc, argv, 2);
    if (args.help || args.positional.size() != 1) {
        print_command_usage("scan");
        return args.help ? 0 : kExitError;
    }

    std::unique_ptr<FingerprintStore> owned_store;
    const FingerprintStore* store = nullptr;
    const auto database_path = resolve_database_path(args);
    if (database_path.has_value()) {
        owned_store = open_store(*database_path);
        if (owned_store == nullptr) {
            return kExitError;
        }
        store = owned_store.get();
    }

    const auto config_path = resolve_classifier_config(args);
    const HashMatchConfig match_config = resolve_match_config(args);
    const auto result = scan_file(args.positional.front(), store, config_path, match_config);
    if (!result.has_value()) {
        std::cerr << "Failed to decode image: " << args.positional.front() << '\n';
        return kExitError;
    }

    const PolicyEvaluation policy = evaluate_policy(*result);

    if (args.json_output) {
        std::cout << '{';
        write_json_string(std::cout, "image");
        std::cout << ':';
        write_json_string(std::cout, args.positional.front());
        std::cout << ',';
        write_json_string(std::cout, "decision");
        std::cout << ':';
        write_json_string(std::cout, scan_decision_label(policy.decision));
        std::cout << ',';
        write_json_string(std::cout, "reason");
        std::cout << ':';
        write_json_string(std::cout, moderation_reason_label(policy.reason));
        std::cout << ',';
        write_json_string(std::cout, "match");
        std::cout << ':';
        write_json_string(std::cout, match_status_label(result->match_status));
        std::cout << std::fixed << std::setprecision(6);
        std::cout << ",\"distance\":" << result->nearest_distance;
        std::cout << ",\"classifier_available\":"
                  << (result->classifier_availability == ClassifierAvailability::Available
                          ? "true"
                          : "false");
        if (result->classification.has_value()) {
            std::cout << ',';
            write_json_string(std::cout, "classifier_label");
            std::cout << ':';
            write_json_string(std::cout, result->classification->label);
            std::cout << ",\"classifier_confidence\":" << result->classification->confidence;
            std::cout << ",\"classifier_detected\":"
                      << (result->classification->detected ? "true" : "false");
        }
        std::cout << "}\n";
        return exit_code_for_decision(policy.decision);
    }

    std::cout << "BioPic Scan\n\n";
    std::cout << "Image:\n" << args.positional.front() << "\n\n";
    std::cout << "Fingerprint:\n";
    std::cout << "  Algorithm: biopic\n";
    std::cout << "  Version: " << result->fingerprint.version << '\n';
    std::cout << "  Status: generated\n\n";
    std::cout << "Database:\n";
    if (database_path.has_value()) {
        std::cout << "  Path: " << *database_path << '\n';
    } else {
        std::cout << "  Status: not configured\n";
    }
    std::cout << "  Match: " << match_status_label(result->match_status) << '\n';
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  Distance: " << result->nearest_distance << '\n';
    if (result->matched_record.has_value()) {
        std::cout << "  Record ID: " << result->matched_record->id << '\n';
        std::cout << "  Record Label: " << result->matched_record->label << '\n';
    }
    std::cout << "\nClassifier:\n";
    std::cout << "  Status: " << result->classifier_status << '\n';
    if (result->classification.has_value()) {
        std::cout << "  Label: " << result->classification->label << '\n';
        std::cout << "  Confidence: " << result->classification->confidence << '\n';
        std::cout << "  Detection: " << (result->classification->detected ? "true" : "false")
                  << '\n';
    }
    std::cout << "\nDecision:\n";
    std::cout << "  " << scan_decision_label(policy.decision) << '\n';
    std::cout << "  Reason: " << moderation_reason_label(policy.reason) << '\n';
    return exit_code_for_decision(policy.decision);
}

int run_classify(int argc, char** argv) {
    const ParsedArgs args = parse_args(argc, argv, 2);
    if (args.help || args.positional.size() != 1) {
        print_command_usage("classify");
        return args.help ? 0 : kExitError;
    }

    const auto config_path = resolve_classifier_config(args);
    if (!config_path.has_value()) {
        std::cerr << "Classifier configuration is required. Pass --config CONFIG or set "
                     "BIOPIC_CLASSIFIER_CONFIG.\n";
        return kExitError;
    }

    const auto config = ClassifierConfig::load_from_file(*config_path);
    if (!config.has_value()) {
        std::cerr << "Failed to load classifier configuration: " << *config_path << '\n';
        return kExitError;
    }

    const auto classifier = create_classifier(*config);
    if (classifier == nullptr) {
        std::cerr << "Failed to initialize classifier\n";
        return kExitError;
    }

    ImageDecoder decoder;
    const auto decoded = decoder.decode_file(args.positional.front());
    if (!decoded) {
        std::cerr << "Failed to decode image: " << args.positional.front() << '\n';
        return kExitError;
    }

    ImageView view(decoded->width, decoded->height, decoded->rgb);
    const ClassificationResult classification = classifier->classify(view);
    std::cout << "Label: " << classification.label << '\n';
    std::cout << "Confidence: " << classification.confidence << '\n';
    std::cout << "Detection result: " << (classification.detected ? "detected" : "not detected")
              << '\n';
    return 0;
}

int run_evaluate(int argc, char** argv) {
    const ParsedArgs args = parse_args(argc, argv, 2);
    if (args.help || args.positional.size() != 1) {
        print_command_usage("evaluate");
        return args.help ? 0 : kExitError;
    }

    const std::filesystem::path dataset_root = args.positional.front();
    const auto samples = evaluation::load_manifest(dataset_root);
    if (samples.empty()) {
        std::cerr << "No evaluation samples found in " << dataset_root << '\n';
        std::cerr << "Provide manifest.csv or allow/review/block subdirectories.\n";
        return kExitError;
    }

    std::unique_ptr<FingerprintStore> owned_store;
    const FingerprintStore* store = nullptr;
    const auto database_path = resolve_database_path(args);
    if (database_path.has_value()) {
        owned_store = open_store(*database_path);
        if (owned_store == nullptr) {
            return kExitError;
        }
        store = owned_store.get();
    }

    const auto config_path = resolve_classifier_config(args);
    const HashMatchConfig match_config = resolve_match_config(args);

    evaluation::ConfusionMatrix matrix{};
    std::size_t skipped = 0;

    for (const evaluation::DatasetSample& sample : samples) {
        const auto result = scan_file(sample.image_path, store, config_path, match_config);
        if (!result.has_value()) {
            ++skipped;
            continue;
        }
        const PolicyEvaluation policy = evaluate_policy(*result);
        evaluation::record_prediction(matrix, sample.expected, policy.decision);
    }

    const evaluation::EvaluationReport report = evaluation::compute_report(matrix, skipped);

    std::cout << "BioPic Evaluation\n\n";
    std::cout << "Dataset: " << dataset_root << '\n';
    std::cout << "Samples: " << report.total << " total, " << report.evaluated << " evaluated, "
              << report.skipped << " skipped\n\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Accuracy: " << report.accuracy << "\n\n";
    print_class_metrics("Allow", report.allow);
    std::cout << '\n';
    print_class_metrics("Review", report.review);
    std::cout << '\n';
    print_class_metrics("Block", report.block);
    std::cout << "\nFlagged (review+block) vs rest:\n";
    std::cout << "  precision: " << report.flagged_precision << '\n';
    std::cout << "  recall:    " << report.flagged_recall << '\n';
    std::cout << "  f1:        " << report.flagged_f1 << '\n';
    return 0;
}

int run_benchmark(int argc, char** argv) {
    if (argc >= 3 && std::string_view(argv[2]) == "similarity") {
        const ParsedArgs args = parse_args(argc, argv, 3);
        std::size_t record_count = 10'000;
        if (!args.positional.empty()) {
            record_count = static_cast<std::size_t>(std::stoul(args.positional.front()));
        }

        auto index = create_default_similarity_index();
        for (std::size_t entry = 0; entry < record_count; ++entry) {
            FingerprintRecord record;
            record.id = std::to_string(entry);
            record.fingerprint = make_benchmark_fingerprint(entry);
            record.label = "sample";
            index->add(record);
        }

        Fingerprint query = make_benchmark_fingerprint(42);
        query.bytes[0] = static_cast<std::uint8_t>(query.bytes[0] + 1U);
        const HashMatchConfig config = kDefaultHashMatchConfig;

        const auto start = std::chrono::steady_clock::now();
        constexpr int kIterations = 100;
        for (int iteration = 0; iteration < kIterations; ++iteration) {
            const NearestMatchResult nearest = index->find_nearest(query, config);
            if (!nearest.record.has_value()) {
                std::cerr << "Benchmark failed to find nearest neighbor\n";
                return kExitError;
            }
        }
        const auto end = std::chrono::steady_clock::now();
        const double elapsed_ms =
            std::chrono::duration<double, std::milli>(end - start).count() / kIterations;

        std::cout << "BioPic Benchmark\n\n";
        std::cout << "Mode: similarity nearest-neighbor\n";
        std::cout << "Records: " << record_count << '\n';
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Mean latency: " << elapsed_ms << " ms\n";
        return 0;
    }

    print_command_usage("benchmark");
    return kExitError;
}

int run_database(int argc, char** argv) {
    if (argc < 3) {
        print_command_usage("database");
        return kExitError;
    }

    const std::string subcommand = argv[2];
    const ParsedArgs args = parse_args(argc, argv, 3);

    if (subcommand == "add") {
        if (args.help || args.positional.size() != 1 || !args.label.has_value()) {
            print_command_usage("database add");
            return args.help ? 0 : kExitError;
        }
        const auto database_path = resolve_database_path(args);
        if (!database_path.has_value()) {
            std::cerr << "Missing required --database argument or BIOPIC_DATABASE\n";
            return kExitError;
        }

        const auto fingerprint = fingerprint_from_image(args.positional.front());
        if (!fingerprint.has_value()) {
            std::cerr << "Failed to decode image\n";
            return kExitError;
        }

        auto store = open_store(*database_path);
        if (store == nullptr) {
            return kExitError;
        }

        FingerprintRecord record;
        record.fingerprint = *fingerprint;
        record.label = *args.label;
        if (!store->add(record)) {
            std::cerr << "Failed to store fingerprint\n";
            return kExitError;
        }

        const auto stored = store->find_exact(*fingerprint);
        if (!stored.has_value()) {
            std::cerr << "Failed to retrieve stored fingerprint\n";
            return kExitError;
        }

        std::cout << "Fingerprint stored\n\n";
        std::cout << "ID:\n" << stored->id << "\n\n";
        std::cout << "Label:\n" << stored->label << '\n';
        return 0;
    }

    if (subcommand == "search") {
        if (args.help || args.positional.size() != 1) {
            print_command_usage("database search");
            return args.help ? 0 : kExitError;
        }
        const auto database_path = resolve_database_path(args);
        if (!database_path.has_value()) {
            std::cerr << "Missing required --database argument or BIOPIC_DATABASE\n";
            return kExitError;
        }

        const auto fingerprint = fingerprint_from_image(args.positional.front());
        if (!fingerprint.has_value()) {
            std::cerr << "Failed to decode image\n";
            return kExitError;
        }

        auto store = open_store(*database_path);
        if (store == nullptr) {
            return kExitError;
        }

        const HashMatchConfig match_config = resolve_match_config(args);
        const NearestMatchResult nearest = store->find_nearest(*fingerprint, match_config);
        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Match:\n" << (nearest.exact_match ? "true" : "false") << "\n\n";
        std::cout << "Distance:\n" << nearest.distance << '\n';
        if (nearest.record.has_value()) {
            std::cout << "\nRecord ID:\n" << nearest.record->id << '\n';
            std::cout << "Label:\n" << nearest.record->label << '\n';
        }
        return 0;
    }

    if (subcommand == "stats") {
        if (args.help) {
            print_command_usage("database stats");
            return 0;
        }
        const auto database_path = resolve_database_path(args);
        if (!database_path.has_value()) {
            std::cerr << "Missing required --database argument or BIOPIC_DATABASE\n";
            return kExitError;
        }

        const auto stats = inspect_database(*database_path);
        if (!stats.has_value()) {
            std::cerr << "Failed to inspect database\n";
            return kExitError;
        }

        std::cout << "Database: " << *database_path << '\n';
        std::cout << "Fingerprints: " << stats->fingerprint_count << '\n';
        std::cout << "Bucket entries: " << stats->bucket_count << '\n';
        std::cout << "File size (bytes): " << stats->file_size_bytes << '\n';
        return 0;
    }

    if (subcommand == "vacuum") {
        if (args.help) {
            print_command_usage("database vacuum");
            return 0;
        }
        const auto database_path = resolve_database_path(args);
        if (!database_path.has_value()) {
            std::cerr << "Missing required --database argument or BIOPIC_DATABASE\n";
            return kExitError;
        }

        if (!vacuum_database(*database_path)) {
            std::cerr << "VACUUM failed\n";
            return kExitError;
        }
        std::cout << "Database vacuum complete: " << *database_path << '\n';
        return 0;
    }

    print_command_usage("database");
    return kExitError;
}

int run_model(int argc, char** argv) {
    if (argc < 3) {
        print_command_usage("model");
        return kExitError;
    }

    const std::string subcommand = argv[2];
    const ParsedArgs args = parse_args(argc, argv, 3);

    if (subcommand == "list") {
        std::cout << "Supported classifier kinds:\n";
        std::cout << "  nudity\n";
        std::cout << "  violence\n";
        std::cout << "  weapons\n";
        std::cout << "  drugs\n";
        std::cout << "  child_safety\n";
        return 0;
    }

    if (subcommand == "info") {
        const auto config_path = resolve_classifier_config(args);
        if (!config_path.has_value()) {
            std::cerr << "Missing --config or BIOPIC_CLASSIFIER_CONFIG\n";
            return kExitError;
        }
        const auto config = ClassifierConfig::load_from_file(*config_path);
        if (!config.has_value()) {
            std::cerr << "Invalid classifier config\n";
            return kExitError;
        }

        std::cout << "Config: " << *config_path << '\n';
        std::cout << "Kind: " << classifier_kind_to_string(config->kind) << '\n';
        std::cout << "Model path: " << config->model_path << '\n';
        std::cout << "Manifest path: " << config->manifest_path << '\n';
        std::cout << "Model exists: "
                  << (std::filesystem::exists(config->model_path) ? "yes" : "no") << '\n';
        return 0;
    }

    if (subcommand == "verify") {
        const auto config_path = resolve_classifier_config(args);
        if (!config_path.has_value()) {
            std::cerr << "Missing --config or BIOPIC_CLASSIFIER_CONFIG\n";
            return kExitError;
        }
        const auto config = ClassifierConfig::load_from_file(*config_path);
        if (!config.has_value()) {
            std::cerr << "Invalid classifier config\n";
            return kExitError;
        }
        if (!std::filesystem::exists(config->model_path)) {
            std::cerr << "Model file not found: " << config->model_path << '\n';
            return kExitError;
        }

        const auto classifier = create_classifier(*config);
        if (classifier == nullptr) {
            std::cerr << "Classifier initialization failed\n";
            return kExitError;
        }

        std::cout << "Model verified: " << config->model_path << '\n';
        std::cout << "Kind: " << classifier_kind_to_string(classifier->kind()) << '\n';
        return 0;
    }

    print_command_usage("model");
    return kExitError;
}

} // namespace biopic::cli
