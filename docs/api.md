# BioPic API Reference

This document describes the primary public C++ API. For the C ABI, see [bindings/c/include/biopic.h](../bindings/c/include/biopic.h).

## Image decoding

### `ImageDecoder`

Decodes encoded image bytes to canonical 8-bit RGB row-major format.

```cpp
#include "biopic/image.hpp"

biopic::ImageDecoder decoder;
const auto decoded = decoder.decode_file("photo.jpg");
if (!decoded) { /* handle error */ }

biopic::ImageView view(decoded->width, decoded->height, decoded->rgb);
```

| Method | Description |
|--------|-------------|
| `decode_file(path)` | Read and decode a file from disk |
| `decode(bytes, size)` | Decode from an in-memory buffer |

`DecodeLimits` controls maximum dimensions, pixel count, and decode timeout. All image bytes are treated as untrusted input.

---

## Hashing

### `Hasher`

Computes a versioned BioPicHash perceptual fingerprint from a decoded image.

```cpp
#include "biopic/hasher.hpp"

biopic::Hasher hasher;
const biopic::Fingerprint fingerprint = hasher.compute(view);
```

| Method | Description |
|--------|-------------|
| `compute(image)` | Returns a 144-byte `Fingerprint` |
| `version()` | Hash algorithm version (currently `1`) |

### `Fingerprint`

```cpp
#include "biopic/fingerprint.hpp"

// Encode for storage or transport
const std::string hex =
    biopic::encode_fingerprint(fp, biopic::FingerprintEncoding::Hex);
const std::string b64 =
    biopic::encode_fingerprint(fp, biopic::FingerprintEncoding::Base64);

// Decode
const auto parsed = biopic::decode_fingerprint("biopic:v1:...", biopic::FingerprintEncoding::Hex);
```

### Distance

```cpp
#include "biopic/distance.hpp"

const double d = biopic::fingerprint_distance(a, b, biopic::DistanceMetric::L2);
```

Supported metrics: `L1`, `L2`, `L2Squared`.

---

## SimilarityIndex

Abstract interface for in-memory fingerprint similarity search. Backends are swappable without changing the scanner or store.

```cpp
#include "biopic/index/similarity_index.hpp"

auto index = biopic::create_default_similarity_index(); // BucketedIndex + LSH

biopic::FingerprintRecord record;
record.id = "abc123";
record.fingerprint = fp;
record.label = "blocked";
index->add(record);

biopic::HashMatchConfig config;
config.threshold = 10.0;
config.metric = biopic::DistanceMetric::L2;

const auto nearest = index->find_nearest(query_fp, config);
const auto similar = index->query(query_fp, config);
```

### Methods

| Method | Description |
|--------|-------------|
| `add(record)` | Insert a labeled fingerprint. Returns `false` if label is empty. |
| `find_nearest(fp, config)` | Return the closest record and distance. Used by the scan pipeline. |
| `query(fp, config, limit)` | Return all records within `config.threshold`, sorted by distance. |
| `size()` | Number of indexed records. |

### Factory functions

| Function | Backend | Use case |
|----------|---------|----------|
| `create_brute_force_index()` | Linear O(n) scan | Tests, correctness baseline |
| `create_bucketed_index()` | LSH bucket index | Production in-memory |
| `create_default_similarity_index()` | Same as `create_bucketed_index()` | Default for stores |

### `HashMatchConfig`

```cpp
struct HashMatchConfig {
    DistanceMetric metric = DistanceMetric::L2;
    double threshold = 10.0;
    bool use_normalized = false;
    std::size_t max_results = 0;  // 0 = unlimited
};
```

Thresholds are deployment-specific and should be calibrated against labeled validation data.

### `NearestMatchResult`

```cpp
struct NearestMatchResult {
    bool exact_match = false;
    double distance = 0.0;
    std::optional<FingerprintRecord> record;
};
```

---

## FingerprintStore

Abstract interface for labeled fingerprint persistence and retrieval. Hides whether storage is in-memory or SQLite-backed.

```cpp
#include "biopic/database/fingerprint_store.hpp"

// Persistent SQLite store
auto store = biopic::open_persistent_fingerprint_store("fingerprints.db");

// In-memory store (tests, ephemeral use)
biopic::InMemoryFingerprintStore memory_store;
```

### Methods

| Method | Description |
|--------|-------------|
| `add(record)` | Insert or replace a labeled fingerprint. Auto-generates id if empty. |
| `find_exact(fp)` | Return a record with an identical fingerprint blob, if any. |
| `find_nearest(fp)` / `find_nearest(fp, config)` | Nearest neighbor via LSH index. |
| `find_similar(fp, threshold)` / `find_similar(fp, config)` | All matches within threshold. |
| `size()` | Record count. |

### `FingerprintRecord`

```cpp
struct FingerprintRecord {
    std::string id;
    Fingerprint fingerprint;
    std::string label;
    std::chrono::system_clock::time_point created_at;
};
```

### `PersistentFingerprintStore`

SQLite implementation. On open, creates tables and rebuilds the LSH bucket index from stored fingerprints. Bucket keys are written on every `add()`.

The store uses a bloom filter for fast exact-match rejection before hitting SQLite.

---

## Classifier

### `IClassifier`

Abstract interface for image classifiers. The ONNX implementation is production-ready; `DummyClassifier` exists for tests.

```cpp
#include "biopic/ai/classifier.hpp"

biopic::OnnxClassifier classifier(
    biopic::ClassifierKind::Nudity,
    biopic::load_model_from_config("models/nudity.json"));
const biopic::ClassificationResult result = classifier.classify(view);
```

| Method | Description |
|--------|-------------|
| `kind()` | Classifier category (`Nudity`, `Violence`, etc.) |
| `model()` | Model metadata and ONNX path |
| `classify(image)` | Run inference; return structured scores |

### `ClassificationResult`

```cpp
struct ClassificationScores {
    double safe = 0.0;
    double suggestive = 0.0;
    double partial_nudity = 0.0;
    double explicit_nudity = 0.0;
    double uncertain = 0.0;
};
```

Classifier configuration is loaded from JSON via `ClassifierConfig` (`biopic/ai/classifier_config.hpp`).

---

## Policy engine

### `ModerationPolicy`

Maps detection signals from `ScanResult` to a moderation decision. Separates **what was detected** from **what to do about it**.

```cpp
#include "biopic/policy/moderation_policy.hpp"
#include "pipeline/scanner.hpp"

const biopic::ScanResult result = biopic::scan(view, store.get(), config_path);
const biopic::PolicyEvaluation policy = biopic::evaluate_policy(result);

// policy.decision  → Allow, Review, or Block
// policy.reason    → KnownHashMatch, SimilarHashMatch, ModelClassification, ...
```

### Default rules (`kDefaultPolicyConfig`)

| Signal | Decision |
|--------|----------|
| Exact fingerprint match | BLOCK |
| Classifier confidence ≥ 0.97 (detected) | BLOCK |
| Classifier confidence 0.70–0.97 (detected) | REVIEW |
| Similar fingerprint match | REVIEW |
| Otherwise | ALLOW |

When multiple signals apply, the highest-severity decision wins.

### `PolicyConfig`

```cpp
struct PolicyConfig {
    double classifier_block_threshold = 0.97;
    double classifier_review_threshold = 0.70;
    ModerationDecision exact_match_decision = ModerationDecision::Block;
    ModerationDecision similar_match_decision = ModerationDecision::Review;
    ModerationDecision default_decision = ModerationDecision::Allow;
};
```

Pass a custom config to `evaluate_policy(result, config)` or construct `ModerationPolicy(config)`.

### Helpers

| Function | Description |
|----------|-------------|
| `evaluate_policy(result, config)` | Full evaluation with reason |
| `scan_decision(result)` | Decision only (default policy) |
| `moderation_reason_label(reason)` | Human-readable reason string |

---

## Scanner

Orchestrates the full moderation path: decode → fingerprint → database lookup → classification.

```cpp
#include "pipeline/scanner.hpp"

const auto result = biopic::scan_file(
    "photo.jpg",
    store.get(),                          // optional FingerprintStore
    std::filesystem::path("model.json"),  // optional classifier config
    biopic::kDefaultHashMatchConfig);

const biopic::PolicyEvaluation policy = biopic::evaluate_policy(result);
const biopic::ModerationDecision decision = policy.decision;
```

### `scan()` overloads

| Signature | Description |
|-----------|-------------|
| `scan_file(path, store, config_path, match_config)` | Decode from disk, then scan |
| `scan(image, store, config_path, match_config)` | Scan a decoded `ImageView` |
| `scan(image, classifier, store, match_config)` | Scan with an explicit classifier instance |

### `ScanResult`

```cpp
struct ScanResult {
    Fingerprint fingerprint;
    std::optional<ClassificationResult> classification;
    ClassifierAvailability classifier_availability;
    std::string classifier_status;
    MatchStatus match_status;           // NoMatch, ExactMatch, SimilarMatch
    std::optional<FingerprintRecord> matched_record;
    double nearest_distance;
};
```

### Decision helpers

```cpp
biopic::PolicyEvaluation policy = biopic::evaluate_policy(result);
biopic::ModerationDecision decision = policy.decision;

const char* label = biopic::scan_decision_label(policy.decision);
const char* reason = biopic::moderation_reason_label(policy.reason);
const char* match = biopic::match_status_label(result.match_status);
```

`evaluate_policy()` applies `ModerationPolicy` rules. See [Policy engine](#policy-engine) above.

---

## Error handling

BioPic uses `std::optional` for recoverable failures (decode errors, missing files) and boolean returns for store operations. The C ABI exposes error codes — see `biopic.h`.

---

## Thread safety

- `Hasher`, `ImageDecoder`: stateless; safe to use concurrently.
- `FingerprintStore` / `SimilarityIndex`: not thread-safe; external synchronization required for concurrent writes.
- `OnnxClassifier`: one session per instance; do not share across threads without locking.

---

## Related documents

- [architecture.md](architecture.md) — system design and module boundaries
- [performance.md](performance.md) — similarity search benchmarks
- [adr/0001-biopichash-v1.md](adr/0001-biopichash-v1.md) — hash algorithm specification
