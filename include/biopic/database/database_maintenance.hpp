#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>

namespace biopic {

struct DatabaseStats {
    std::size_t fingerprint_count = 0;
    std::size_t bucket_count = 0;
    std::uintmax_t file_size_bytes = 0;
};

[[nodiscard]] std::optional<DatabaseStats> inspect_database(const std::filesystem::path& database);

[[nodiscard]] bool vacuum_database(const std::filesystem::path& database);

} // namespace biopic
