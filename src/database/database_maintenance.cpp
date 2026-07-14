#include "biopic/database/database_maintenance.hpp"

#include <sqlite3.h>

namespace biopic {

namespace {

bool execute_sql(sqlite3* database, const char* sql) {
    char* error_message = nullptr;
    const int result = sqlite3_exec(database, sql, nullptr, nullptr, &error_message);
    if (result != SQLITE_OK) {
        sqlite3_free(error_message);
        return false;
    }
    return true;
}

std::optional<DatabaseStats> query_stats(sqlite3* database,
                                         const std::filesystem::path& database_path) {
    DatabaseStats stats;
    if (std::filesystem::exists(database_path)) {
        stats.file_size_bytes = std::filesystem::file_size(database_path);
    }

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, "SELECT COUNT(*) FROM fingerprints;", -1, &statement,
                           nullptr) == SQLITE_OK) {
        if (sqlite3_step(statement) == SQLITE_ROW) {
            stats.fingerprint_count =
                static_cast<std::size_t>(sqlite3_column_int64(statement, 0));
        }
        sqlite3_finalize(statement);
    }

    if (sqlite3_prepare_v2(database, "SELECT COUNT(*) FROM fingerprint_buckets;", -1, &statement,
                           nullptr) == SQLITE_OK) {
        if (sqlite3_step(statement) == SQLITE_ROW) {
            stats.bucket_count = static_cast<std::size_t>(sqlite3_column_int64(statement, 0));
        }
        sqlite3_finalize(statement);
    }

    return stats;
}

} // namespace

std::optional<DatabaseStats> inspect_database(const std::filesystem::path& database) {
    sqlite3* handle = nullptr;
    if (sqlite3_open(database.string().c_str(), &handle) != SQLITE_OK) {
        if (handle != nullptr) {
            sqlite3_close(handle);
        }
        return std::nullopt;
    }

    const auto stats = query_stats(handle, database);
    sqlite3_close(handle);
    return stats;
}

bool vacuum_database(const std::filesystem::path& database) {
    sqlite3* handle = nullptr;
    if (sqlite3_open(database.string().c_str(), &handle) != SQLITE_OK) {
        if (handle != nullptr) {
            sqlite3_close(handle);
        }
        return false;
    }

    const bool success = execute_sql(handle, "VACUUM;");
    sqlite3_close(handle);
    return success;
}

} // namespace biopic
