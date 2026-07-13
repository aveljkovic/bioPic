#include "biopic/database/fingerprint_store.hpp"

#include "biopic/index/similarity_index.hpp"

#include <sstream>

namespace biopic {

namespace {

std::vector<FingerprintRecord> records_from_query_results(
    const std::vector<SimilarityQueryResult>& results) {
    std::vector<FingerprintRecord> records;
    records.reserve(results.size());
    for (const SimilarityQueryResult& result : results) {
        records.push_back(result.matched);
    }
    return records;
}

} // namespace

bool fingerprints_equal(const Fingerprint& left, const Fingerprint& right) {
    return left.algorithm == right.algorithm && left.version == right.version &&
           left.bytes == right.bytes;
}

std::vector<FingerprintRecord> find_similar_among_records(
    const std::vector<FingerprintRecord>& records, const Fingerprint& fingerprint,
    double threshold) {
    BruteForceIndex index;
    for (const FingerprintRecord& entry : records) {
        index.add(entry);
    }

    HashMatchConfig config = kDefaultHashMatchConfig;
    config.threshold = threshold;
    return records_from_query_results(index.query(fingerprint, config));
}

NearestMatchResult find_nearest_among_records(const std::vector<FingerprintRecord>& records,
                                              const Fingerprint& fingerprint) {
    BruteForceIndex index;
    for (const FingerprintRecord& entry : records) {
        index.add(entry);
    }
    return index.find_nearest(fingerprint, kDefaultHashMatchConfig);
}

InMemoryFingerprintStore::InMemoryFingerprintStore() : index_(create_default_similarity_index()) {}

InMemoryFingerprintStore::~InMemoryFingerprintStore() = default;

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
    return index_->add(record);
}

std::optional<FingerprintRecord> InMemoryFingerprintStore::find_exact(
    const Fingerprint& fingerprint) const {
    const NearestMatchResult nearest = index_->find_nearest(fingerprint, kDefaultHashMatchConfig);
    if (nearest.exact_match && nearest.record.has_value()) {
        return nearest.record;
    }
    return std::nullopt;
}

std::vector<FingerprintRecord> InMemoryFingerprintStore::find_similar(
    const Fingerprint& fingerprint, double threshold) const {
    HashMatchConfig config = kDefaultHashMatchConfig;
    config.threshold = threshold;
    return find_similar(fingerprint, config);
}

std::vector<FingerprintRecord> InMemoryFingerprintStore::find_similar(
    const Fingerprint& fingerprint, const HashMatchConfig& config) const {
    return records_from_query_results(index_->query(fingerprint, config));
}

NearestMatchResult InMemoryFingerprintStore::find_nearest(const Fingerprint& fingerprint) const {
    return find_nearest(fingerprint, kDefaultHashMatchConfig);
}

NearestMatchResult InMemoryFingerprintStore::find_nearest(
    const Fingerprint& fingerprint, const HashMatchConfig& config) const {
    return index_->find_nearest(fingerprint, config);
}

std::size_t InMemoryFingerprintStore::size() const noexcept {
    return index_->size();
}

} // namespace biopic
