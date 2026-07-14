#include "biopic/evaluation/dataset_manifest.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace biopic::evaluation {

namespace {

std::string trim(std::string value) {
    const auto not_space = [](unsigned char character) { return !std::isspace(character); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
}

void append_unique_sample(std::vector<DatasetSample>& samples, DatasetSample sample) {
    for (const DatasetSample& existing : samples) {
        if (existing.image_path == sample.image_path) {
            return;
        }
    }
    samples.push_back(std::move(sample));
}

void load_from_manifest_file(const std::filesystem::path& manifest_path,
                             const std::filesystem::path& dataset_root,
                             std::vector<DatasetSample>& samples) {
    std::ifstream input(manifest_path);
    if (!input.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const std::size_t comma = line.find(',');
        if (comma == std::string::npos) {
            continue;
        }

        const std::string path_text = trim(line.substr(0, comma));
        const std::string expected_text = trim(line.substr(comma + 1));
        const auto expected = parse_expected_decision(expected_text);
        if (!expected.has_value() || path_text.empty()) {
            continue;
        }

        DatasetSample sample;
        sample.image_path = dataset_root / path_text;
        sample.expected = *expected;
        append_unique_sample(samples, std::move(sample));
    }
}

void load_from_label_directories(const std::filesystem::path& dataset_root,
                                 std::vector<DatasetSample>& samples) {
    static constexpr const char* kLabelNames[] = {"allow", "review", "block"};
    static constexpr ModerationDecision kDecisions[] = {ModerationDecision::Allow,
                                                        ModerationDecision::Review,
                                                        ModerationDecision::Block};

    for (std::size_t index = 0; index < 3; ++index) {
        const std::filesystem::path label_dir = dataset_root / kLabelNames[index];
        if (!std::filesystem::is_directory(label_dir)) {
            continue;
        }

        for (const auto& entry : std::filesystem::directory_iterator(label_dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            DatasetSample sample;
            sample.image_path = entry.path();
            sample.expected = kDecisions[index];
            append_unique_sample(samples, std::move(sample));
        }
    }
}

} // namespace

std::optional<ModerationDecision> parse_expected_decision(std::string_view text) {
    const std::string normalized = lowercase(std::string(text));
    if (normalized == "allow" || normalized == "safe") {
        return ModerationDecision::Allow;
    }
    if (normalized == "review" || normalized == "flag" || normalized == "flagged") {
        return ModerationDecision::Review;
    }
    if (normalized == "block" || normalized == "blocked" || normalized == "bad") {
        return ModerationDecision::Block;
    }
    return std::nullopt;
}

std::vector<DatasetSample> load_manifest(const std::filesystem::path& dataset_root) {
    std::vector<DatasetSample> samples;
    const std::filesystem::path csv_manifest = dataset_root / "manifest.csv";
    const std::filesystem::path tsv_manifest = dataset_root / "manifest.tsv";

    if (std::filesystem::is_regular_file(csv_manifest)) {
        load_from_manifest_file(csv_manifest, dataset_root, samples);
    } else if (std::filesystem::is_regular_file(tsv_manifest)) {
        load_from_manifest_file(tsv_manifest, dataset_root, samples);
    }

    if (samples.empty()) {
        load_from_label_directories(dataset_root, samples);
    }

    std::sort(samples.begin(), samples.end(),
              [](const DatasetSample& left, const DatasetSample& right) {
                  return left.image_path.string() < right.image_path.string();
              });
    return samples;
}

} // namespace biopic::evaluation
