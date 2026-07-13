#include "biopic/database/fingerprint_store.hpp"

#include "biopic/distance.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

namespace biopic {

namespace {

struct RankedRecord {
    FingerprintRecord record;
    double distance = 0.0;
};

} // namespace

bool fingerprints_equal(const Fingerprint& left, const Fingerprint& right) {
    return left.algorithm == right.algorithm && left.version == right.version &&
           left.bytes == right.bytes;
}

std::vector<FingerprintRecord> find_similar_among_records(
    const std::vector<FingerprintRecord>& records, const Fingerprint& fingerprint,
    double threshold) {
    std::vector<RankedRecord> ranked;
    ranked.reserve(records.size());

    for (const FingerprintRecord& record : records) {
        const double distance = l2_distance(fingerprint.bytes, record.fingerprint.bytes);
        if (distance <= threshold) {
            ranked.push_back(RankedRecord{record, distance});
        }
    }

    std::sort(ranked.begin(), ranked.end(),
              [](const RankedRecord& left, const RankedRecord& right) {
                  return left.distance < right.distance;
              });

    std::vector<FingerprintRecord> matches;
    matches.reserve(ranked.size());
    for (const RankedRecord& entry : ranked) {
        matches.push_back(entry.record);
    }
    return matches;
}

NearestMatchResult find_nearest_among_records(const std::vector<FingerprintRecord>& records,
                                              const Fingerprint& fingerprint) {
    NearestMatchResult result;
    if (records.empty()) {
        return result;
    }

    for (const FingerprintRecord& record : records) {
        if (fingerprints_equal(record.fingerprint, fingerprint)) {
            result.exact_match = true;
            result.distance = 0.0;
            result.record = record;
            return result;
        }
    }

    double best_distance = std::numeric_limits<double>::infinity();
    std::optional<FingerprintRecord> best_record;
    for (const FingerprintRecord& record : records) {
        const double distance = l2_distance(fingerprint.bytes, record.fingerprint.bytes);
        if (distance < best_distance) {
            best_distance = distance;
            best_record = record;
        }
    }

    result.exact_match = false;
    result.distance = best_distance;
    result.record = best_record;
    return result;
}

std::string InMemoryFingerprintStore::generate_id() {
    std::ostringstream stream;
    stream << std::hex << next_id_++;
    return stream.str();
}

bool InMemoryFingerprintStore::add(FingerprintRecord record) {
    if (record.id.empty()) {
        record.id = generate_id();
    }
    if (record.label.empty()) {
        return false;
    }
    records_.push_back(std::move(record));
    return true;
}

std::optional<FingerprintRecord> InMemoryFingerprintStore::find_exact(
    const Fingerprint& fingerprint) const {
    for (const FingerprintRecord& record : records_) {
        if (fingerprints_equal(record.fingerprint, fingerprint)) {
            return record;
        }
    }
    return std::nullopt;
}

std::vector<FingerprintRecord> InMemoryFingerprintStore::find_similar(
    const Fingerprint& fingerprint, double threshold) const {
    return find_similar_among_records(records_, fingerprint, threshold);
}

NearestMatchResult InMemoryFingerprintStore::find_nearest(const Fingerprint& fingerprint) const {
    return find_nearest_among_records(records_, fingerprint);
}

} // namespace biopic
