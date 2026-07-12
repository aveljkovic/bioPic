#pragma once

#include <optional>
#include <string>
#include <vector>

#include "biopic/distance.hpp"
#include "biopic/fingerprint.hpp"
#include "biopic/types.hpp"

namespace biopic {

struct StoredHash {
    std::string id;
    std::string tenant_id;
    Fingerprint fingerprint;
    std::string sha256_hex;
    std::string status;
};

struct HashSearchResult {
    bool matched = false;
    std::optional<StoredHash> hash;
    DistanceResult distance;
    double threshold = 0.0;
};

// Abstract interface for persistent known-hash storage and retrieval.
// No concrete implementation is provided in Milestone 1.
class IHashStore {
  public:
    virtual ~IHashStore() = default;

    virtual std::optional<StoredHash> find_by_sha256(const std::string& tenant_id,
                                                     const std::string& sha256_hex) = 0;

    virtual std::optional<StoredHash>
    find_by_fingerprint_exact(const std::string& tenant_id, const Fingerprint& fingerprint) = 0;

    virtual std::vector<StoredHash>
    find_near_candidates(const std::string& tenant_id, const Fingerprint& fingerprint,
                         std::size_t limit) = 0;
};

} // namespace biopic
