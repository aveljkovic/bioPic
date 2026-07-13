#include "biopic/database/sqlite_fingerprint_store.hpp"

#include "biopic/database/fingerprint_store.hpp"
#include "biopic/index/similarity_index.hpp"

#include <sqlite3.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <utility>

namespace biopic {

namespace {

constexpr const char* kCreateTableSql = R"SQL(
CREATE TABLE IF NOT EXISTS fingerprints (
    id TEXT PRIMARY KEY,
    fingerprint BLOB NOT NULL,
    label TEXT NOT NULL,
    created_at INTEGER NOT NULL
);
)SQL";

constexpr const char* kCreateIndexSql =
    "CREATE INDEX IF NOT EXISTS idx_fingerprint ON fingerprints(fingerprint);";

constexpr std::size_t kFingerprintBlobSize =
    sizeof(std::uint32_t) + sizeof(std::uint8_t) + kFingerprintComponentCount;

std::vector<std::uint8_t> encode_fingerprint_blob(const Fingerprint& fingerprint) {
    std::vector<std::uint8_t> blob(kFingerprintBlobSize);
    std::memcpy(blob.data(), &fingerprint.version, sizeof(fingerprint.version));
    blob[sizeof(std::uint32_t)] = static_cast<std::uint8_t>(fingerprint.algorithm);
    std::memcpy(blob.data() + sizeof(std::uint32_t) + sizeof(std::uint8_t),
                fingerprint.bytes.data(), fingerprint.bytes.size());
    return blob;
}

bool decode_fingerprint_blob(const void* data, int size, Fingerprint& fingerprint) {
    if (data == nullptr || size != static_cast<int>(kFingerprintBlobSize)) {
        return false;
    }

    const auto* bytes = static_cast<const std::uint8_t*>(data);
    std::memcpy(&fingerprint.version, bytes, sizeof(fingerprint.version));
    fingerprint.algorithm = static_cast<HashAlgorithm>(bytes[sizeof(std::uint32_t)]);
    std::memcpy(fingerprint.bytes.data(),
                bytes + sizeof(std::uint32_t) + sizeof(std::uint8_t),
                fingerprint.bytes.size());
    return true;
}

std::int64_t to_unix_timestamp(const std::chrono::system_clock::time_point& timestamp) {
    return std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count();
}

std::chrono::system_clock::time_point from_unix_timestamp(std::int64_t timestamp) {
    return std::chrono::system_clock::time_point{std::chrono::seconds(timestamp)};
}

FingerprintRecord row_to_record(sqlite3_stmt* statement) {
    FingerprintRecord record;
    const auto* id = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
    record.id = id != nullptr ? id : "";
    decode_fingerprint_blob(sqlite3_column_blob(statement, 1), sqlite3_column_bytes(statement, 1),
                            record.fingerprint);
    const auto* label = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));
    record.label = label != nullptr ? label : "";
    record.created_at = from_unix_timestamp(sqlite3_column_int64(statement, 3));
    return record;
}

bool execute_sql(sqlite3* database, const char* sql) {
    char* error_message = nullptr;
    const int result = sqlite3_exec(database, sql, nullptr, nullptr, &error_message);
    if (result != SQLITE_OK) {
        sqlite3_free(error_message);
        return false;
    }
    return true;
}

} // namespace

struct PersistentFingerprintStore::Impl {
    sqlite3* database = nullptr;
    std::filesystem::path path;
    std::unique_ptr<SimilarityIndex> similarity_index;
};

PersistentFingerprintStore::PersistentFingerprintStore() : impl_(std::make_unique<Impl>()) {}

PersistentFingerprintStore::~PersistentFingerprintStore() {
    if (impl_->database != nullptr) {
        sqlite3_close(impl_->database);
        impl_->database = nullptr;
    }
}

bool PersistentFingerprintStore::open(const std::filesystem::path& database) {
    if (impl_->database != nullptr) {
        sqlite3_close(impl_->database);
        impl_->database = nullptr;
    }

    const int open_result =
        sqlite3_open(database.string().c_str(), &impl_->database);
    if (open_result != SQLITE_OK) {
        if (impl_->database != nullptr) {
            sqlite3_close(impl_->database);
            impl_->database = nullptr;
        }
        return false;
    }

    if (!execute_sql(impl_->database, kCreateTableSql) ||
        !execute_sql(impl_->database, kCreateIndexSql)) {
        sqlite3_close(impl_->database);
        impl_->database = nullptr;
        return false;
    }

    impl_->path = database;
    return rebuild_similarity_index();
}

bool PersistentFingerprintStore::rebuild_similarity_index() {
    impl_->similarity_index = create_brute_force_index();
    const std::vector<FingerprintRecord> records = load_all_records();
    for (const FingerprintRecord& candidate : records) {
        if (!impl_->similarity_index->add(candidate)) {
            return false;
        }
    }
    return true;
}

bool PersistentFingerprintStore::is_open() const noexcept {
    return impl_->database != nullptr;
}

bool PersistentFingerprintStore::add(FingerprintRecord record) {
    if (!is_open() || record.label.empty()) {
        return false;
    }

    if (record.id.empty()) {
        const std::size_t next_id = size() + 1;
        std::ostringstream stream;
        stream << std::hex << next_id;
        record.id = stream.str();
    }

    const auto blob = encode_fingerprint_blob(record.fingerprint);
    const std::int64_t created_at = to_unix_timestamp(record.created_at);

    sqlite3_stmt* statement = nullptr;
    constexpr const char* kInsertSql =
        "INSERT INTO fingerprints (id, fingerprint, label, created_at) VALUES (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(impl_->database, kInsertSql, -1, &statement, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(statement, 1, record.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(statement, 2, blob.data(), static_cast<int>(blob.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, record.label.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(statement, 4, created_at);

    const int step_result = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (step_result != SQLITE_DONE) {
        return false;
    }

    return impl_->similarity_index->add(record);
}

std::optional<FingerprintRecord> PersistentFingerprintStore::find_exact(
    const Fingerprint& fingerprint) const {
    if (!is_open()) {
        return std::nullopt;
    }

    const auto blob = encode_fingerprint_blob(fingerprint);
    sqlite3_stmt* statement = nullptr;
    constexpr const char* kSelectSql =
        "SELECT id, fingerprint, label, created_at FROM fingerprints WHERE fingerprint = ? "
        "LIMIT 1;";
    if (sqlite3_prepare_v2(impl_->database, kSelectSql, -1, &statement, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_blob(statement, 1, blob.data(), static_cast<int>(blob.size()), SQLITE_TRANSIENT);
    std::optional<FingerprintRecord> record;
    if (sqlite3_step(statement) == SQLITE_ROW) {
        record = row_to_record(statement);
    }
    sqlite3_finalize(statement);
    return record;
}

std::vector<FingerprintRecord> PersistentFingerprintStore::load_all_records() const {
    std::vector<FingerprintRecord> records;
    if (!is_open()) {
        return records;
    }

    sqlite3_stmt* statement = nullptr;
    constexpr const char* kSelectAllSql =
        "SELECT id, fingerprint, label, created_at FROM fingerprints;";
    if (sqlite3_prepare_v2(impl_->database, kSelectAllSql, -1, &statement, nullptr) != SQLITE_OK) {
        return records;
    }

    while (sqlite3_step(statement) == SQLITE_ROW) {
        records.push_back(row_to_record(statement));
    }
    sqlite3_finalize(statement);
    return records;
}

std::vector<FingerprintRecord> PersistentFingerprintStore::find_similar(
    const Fingerprint& fingerprint, double threshold) const {
    HashMatchConfig config = kDefaultHashMatchConfig;
    config.threshold = threshold;
    return find_similar(fingerprint, config);
}

std::vector<FingerprintRecord> PersistentFingerprintStore::find_similar(
    const Fingerprint& fingerprint, const HashMatchConfig& config) const {
    if (!is_open()) {
        return {};
    }

    std::vector<FingerprintRecord> matches;
    for (const SimilarityQueryResult& result : impl_->similarity_index->query(fingerprint, config)) {
        matches.push_back(result.matched);
    }
    return matches;
}

NearestMatchResult PersistentFingerprintStore::find_nearest(
    const Fingerprint& fingerprint) const {
    return find_nearest(fingerprint, kDefaultHashMatchConfig);
}

NearestMatchResult PersistentFingerprintStore::find_nearest(
    const Fingerprint& fingerprint, const HashMatchConfig& config) const {
    if (!is_open()) {
        return {};
    }

    if (const auto exact = find_exact(fingerprint); exact.has_value()) {
        NearestMatchResult result;
        result.exact_match = true;
        result.distance = 0.0;
        result.record = exact;
        return result;
    }

    return impl_->similarity_index->find_nearest(fingerprint, config);
}

std::size_t PersistentFingerprintStore::size() const {
    if (!is_open()) {
        return 0;
    }

    sqlite3_stmt* statement = nullptr;
    constexpr const char* kCountSql = "SELECT COUNT(*) FROM fingerprints;";
    if (sqlite3_prepare_v2(impl_->database, kCountSql, -1, &statement, nullptr) != SQLITE_OK) {
        return 0;
    }

    std::size_t count = 0;
    if (sqlite3_step(statement) == SQLITE_ROW) {
        count = static_cast<std::size_t>(sqlite3_column_int64(statement, 0));
    }
    sqlite3_finalize(statement);
    return count;
}

std::unique_ptr<FingerprintStore> open_persistent_fingerprint_store(
    const std::filesystem::path& database_path) {
    auto store = std::make_unique<PersistentFingerprintStore>();
    if (!store->open(database_path)) {
        return nullptr;
    }
    return store;
}

} // namespace biopic
