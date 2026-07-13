#include "biopic/ai/classifier_config.hpp"
#include "biopic/database/fingerprint_store.hpp"
#include "biopic/distance.hpp"
#include "biopic/env.hpp"
#include "biopic/fingerprint.hpp"
#include "biopic/hasher.hpp"
#include "biopic/image.hpp"
#include "pipeline/scanner.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>

namespace {

void print_usage() {
    std::cerr << "Usage:\n"
              << "  biopic hash IMAGE\n"
              << "  biopic compare IMAGE_A IMAGE_B\n"
              << "  biopic classify IMAGE [--config CONFIG]\n"
              << "  biopic scan IMAGE [--config CONFIG]\n"
              << "  biopic database add IMAGE --label LABEL\n"
              << "  biopic database search IMAGE\n";
}

int hash_image(const std::filesystem::path& path) {
    biopic::ImageDecoder decoder;
    const auto decoded = decoder.decode_file(path.string());
    if (!decoded) {
        std::cerr << "Failed to decode image: " << path << '\n';
        return 1;
    }

    biopic::ImageView view(decoded->width, decoded->height, decoded->rgb);
    biopic::Hasher hasher;
    const biopic::Fingerprint fingerprint = hasher.compute(view);

    const std::string hex =
        biopic::encode_fingerprint(fingerprint, biopic::FingerprintEncoding::Hex);
    const std::string base64 =
        biopic::encode_fingerprint(fingerprint, biopic::FingerprintEncoding::Base64);

    std::cout << "algorithm: biopic\n";
    std::cout << "version: " << fingerprint.version << '\n';
    std::cout << "sha256_rgb: " << decoded->sha256_hex << '\n';
    std::cout << "hex: " << hex << '\n';
    std::cout << "base64: " << base64 << '\n';
    return 0;
}

int compare_images(const std::filesystem::path& left, const std::filesystem::path& right) {
    biopic::ImageDecoder decoder;
    const auto decoded_left = decoder.decode_file(left.string());
    const auto decoded_right = decoder.decode_file(right.string());
    if (!decoded_left || !decoded_right) {
        std::cerr << "Failed to decode one or both images\n";
        return 1;
    }

    biopic::ImageView view_left(decoded_left->width, decoded_left->height, decoded_left->rgb);
    biopic::ImageView view_right(decoded_right->width, decoded_right->height, decoded_right->rgb);
    biopic::Hasher hasher;
    const biopic::Fingerprint fingerprint_left = hasher.compute(view_left);
    const biopic::Fingerprint fingerprint_right = hasher.compute(view_right);

    const auto l1 = biopic::compare_fingerprints(fingerprint_left, fingerprint_right,
                                                 biopic::DistanceMetric::L1);
    const auto l2 = biopic::compare_fingerprints(fingerprint_left, fingerprint_right,
                                                 biopic::DistanceMetric::L2);
    const auto l2_squared = biopic::compare_fingerprints(fingerprint_left, fingerprint_right,
                                                         biopic::DistanceMetric::L2Squared);

    std::cout << "l1: " << l1.value << '\n';
    std::cout << "l2: " << l2.value << '\n';
    std::cout << "l2_squared: " << l2_squared.value << '\n';
    return 0;
}

std::optional<std::filesystem::path> resolve_classifier_config(int argc, char** argv) {
    for (int index = 3; index < argc - 1; ++index) {
        const std::string argument = argv[index];
        if (argument == "--config") {
            return std::filesystem::path(argv[index + 1]);
        }
    }

    const auto config_from_env = biopic::read_env_variable("BIOPIC_CLASSIFIER_CONFIG");
    if (config_from_env.has_value()) {
        return std::filesystem::path(*config_from_env);
    }
    return std::nullopt;
}

std::optional<std::string> parse_label_argument(int argc, char** argv, int start_index) {
    for (int index = start_index; index < argc - 1; ++index) {
        const std::string argument = argv[index];
        if (argument == "--label") {
            return std::string(argv[index + 1]);
        }
    }
    return std::nullopt;
}

std::optional<biopic::Fingerprint> fingerprint_from_image(const std::filesystem::path& path) {
    biopic::ImageDecoder decoder;
    const auto decoded = decoder.decode_file(path.string());
    if (!decoded) {
        return std::nullopt;
    }

    biopic::ImageView view(decoded->width, decoded->height, decoded->rgb);
    biopic::Hasher hasher;
    return hasher.compute(view);
}

int classify_image(const std::filesystem::path& path,
                   const std::optional<std::filesystem::path>& config_path) {
    if (!config_path.has_value()) {
        std::cerr << "Classifier configuration is required. Pass --config CONFIG or set "
                     "BIOPIC_CLASSIFIER_CONFIG.\n";
        return 1;
    }

    const auto config = biopic::ClassifierConfig::load_from_file(*config_path);
    if (!config.has_value()) {
        std::cerr << "Failed to load classifier configuration: " << *config_path << '\n';
        return 1;
    }

    const auto classifier = biopic::create_classifier(*config);
    if (classifier == nullptr) {
        std::cerr << "Failed to initialize classifier from configuration: " << *config_path
                  << '\n';
        return 1;
    }

    biopic::ImageDecoder decoder;
    const auto decoded = decoder.decode_file(path.string());
    if (!decoded) {
        std::cerr << "Failed to decode image: " << path << '\n';
        return 1;
    }

    biopic::ImageView view(decoded->width, decoded->height, decoded->rgb);
    const biopic::ClassificationResult result = classifier->classify(view);

    std::cout << "Label: " << result.label << '\n';
    std::cout << "Confidence: " << result.confidence << '\n';
    std::cout << "Detection result: " << (result.detected ? "detected" : "not detected") << '\n';
    return 0;
}

int scan_image(const std::filesystem::path& path,
               const std::optional<std::filesystem::path>& config_path) {
    const auto result =
        biopic::scan_file(path, config_path, &biopic::shared_fingerprint_store());
    if (!result.has_value()) {
        std::cerr << "Failed to decode image: " << path << '\n';
        return 1;
    }

    const biopic::ModerationDecision decision = biopic::scan_decision(*result);

    std::cout << "BioPic Scan\n\n";
    std::cout << "Image:\n" << path.string() << "\n\n";
    std::cout << "Fingerprint:\n";
    std::cout << "  Algorithm: biopic\n";
    std::cout << "  Version: " << result->fingerprint.version << '\n';
    std::cout << "  Status: generated\n\n";
    std::cout << "Database:\n";
    std::cout << "  Match: " << biopic::match_status_label(result->match_status) << '\n';
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
    std::cout << "  " << biopic::scan_decision_label(decision) << '\n';
    return 0;
}

int database_add(const std::filesystem::path& path, const std::string& label) {
    const auto fingerprint = fingerprint_from_image(path);
    if (!fingerprint.has_value()) {
        std::cerr << "Failed to decode image: " << path << '\n';
        return 1;
    }

    biopic::FingerprintRecord record;
    record.fingerprint = *fingerprint;
    record.label = label;
    biopic::FingerprintStore& store = biopic::shared_fingerprint_store();
    if (!store.add(record)) {
        std::cerr << "Failed to store fingerprint\n";
        return 1;
    }

    const auto stored = store.find_exact(*fingerprint);
    if (!stored.has_value()) {
        std::cerr << "Failed to retrieve stored fingerprint\n";
        return 1;
    }

    std::cout << "Fingerprint stored\n\n";
    std::cout << "ID:\n" << stored->id << "\n\n";
    std::cout << "Label:\n" << stored->label << '\n';
    return 0;
}

int database_search(const std::filesystem::path& path) {
    const auto fingerprint = fingerprint_from_image(path);
    if (!fingerprint.has_value()) {
        std::cerr << "Failed to decode image: " << path << '\n';
        return 1;
    }

    const biopic::FingerprintStore& store = biopic::shared_fingerprint_store();
    const biopic::NearestMatchResult nearest = store.find_nearest(*fingerprint);

    std::cout << "Match:\n" << (nearest.exact_match ? "true" : "false") << "\n\n";
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Distance:\n" << nearest.distance << '\n';
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        print_usage();
        return 1;
    }

    const std::string command = argv[1];
    if (command == "hash" && argc == 3) {
        return hash_image(argv[2]);
    }
    if (command == "compare" && argc == 4) {
        return compare_images(argv[2], argv[3]);
    }
    if (command == "classify" && argc >= 3) {
        return classify_image(argv[2], resolve_classifier_config(argc, argv));
    }
    if (command == "scan" && argc >= 3) {
        return scan_image(argv[2], resolve_classifier_config(argc, argv));
    }
    if (command == "database" && argc >= 4) {
        const std::string subcommand = argv[2];
        if (subcommand == "add") {
            const auto label = parse_label_argument(argc, argv, 4);
            if (!label.has_value()) {
                std::cerr << "Missing required --label argument\n";
                print_usage();
                return 1;
            }
            return database_add(argv[3], *label);
        }
        if (subcommand == "search" && argc == 4) {
            return database_search(argv[3]);
        }
    }

    print_usage();
    return 1;
}
