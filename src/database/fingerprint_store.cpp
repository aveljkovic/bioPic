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

std::string FingerprintStore::generate_id() {
    std::ostringstream stream;
    stream << std::hex << next_id_++;
    return stream.str();
}

bool FingerprintStore::add(FingerprintRecord record) {
    if (record.id.empty()) {
        record.id = generate_id();
    }
    if (record.label.empty()) {
        return false;
    }
    records_.push_back(std::move(record));
    return true;
}

std::optional<FingerprintRecord> FingerprintStore::find_exact(
    const Fingerprint& fingerprint) const {
    for (const FingerprintRecord& record : records_) {
        if (fingerprints_equal(record.fingerprint, fingerprint)) {
            return record;
        }
    }
    return std::nullopt;
}

std::vector<FingerprintRecord> FingerprintStore::find_similar(const Fingerprint& fingerprint,
                                                              double threshold) const {
    std::vector<RankedRecord> ranked;
    ranked.reserve(records_.size());

    for (const FingerprintRecord& record : records_) {
        const double distance =
            l2_distance(fingerprint.bytes, record.fingerprint.bytes);
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

NearestMatchResult FingerprintStore::find_nearest(const Fingerprint& fingerprint) const {
    NearestMatchResult result;
    if (records_.empty()) {
        return result;
    }

    if (const auto exact = find_exact(fingerprint); exact.has_value()) {
        result.exact_match = true;
        result.distance = 0.0;
        result.record = exact;
        return result;
    }

    double best_distance = std::numeric_limits<double>::infinity();
    std::optional<FingerprintRecord> best_record;
    for (const FingerprintRecord& record : records_) {
        const double distance =
            l2_distance(fingerprint.bytes, record.fingerprint.bytes);
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

FingerprintStore& shared_fingerprint_store() {
    static FingerprintStore store;
    return store;
}

} // namespace biopic
