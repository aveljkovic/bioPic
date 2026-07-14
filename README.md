# BioPic

BioPic is an open-source image moderation engine for third-party applications. It combines a deterministic perceptual fingerprint (**BioPicHash**), ONNX classification, SQLite-backed hash matching with LSH candidate reduction, and a unified scan pipeline. **Enforcement remains the responsibility of the host application** — BioPic returns structured decisions; it does not delete content or ban users.

Clone it today and you can hash images, classify them, scan against a local fingerprint database, and integrate via C++ or C — offline, with no external services required.

## Features

| Component | Status |
|-----------|--------|
| **BioPicHash v1** — 144-byte perceptual fingerprint | ✅ |
| **Image decoding** — secure decode to canonical RGB (OpenCV) | ✅ |
| **Distance metrics** — L1, L2, L2-squared | ✅ |
| **ONNX classifier** — `OnnxClassifier` with configurable models | ✅ |
| **Scan pipeline** — decode → fingerprint → database → classify → decision | ✅ |
| **SQLite persistence** — `PersistentFingerprintStore` with bucket index | ✅ |
| **Similarity search** — `SimilarityIndex` with LSH candidate reduction | ✅ |
| **CLI** — hash, compare, classify, scan, database commands | ✅ |
| **C ABI** — stable `biopic_c` shared library | ✅ |
| **Tests & benchmarks** — 27 unit/integration tests, Google Benchmark suites | ✅ |

## Architecture

```
                         Image bytes
                              │
                       ImageDecoder
                              │
                          Hasher
                              │
                        Fingerprint
                              │
              ┌───────────────┴───────────────┐
              │                               │
       FingerprintStore                 IClassifier
    (SQLite + LSH index)              (ONNX Runtime)
              │                               │
              └───────────────┬───────────────┘
                              │
                          Scanner
                              │
                    ModerationDecision
                              │
                            CLI / SDK
```

**Data flow for `biopic scan`:**

1. Decode image to canonical 8-bit RGB.
2. Compute BioPicHash v1 fingerprint.
3. If a database is configured, search for exact or nearest fingerprint match (LSH-reduced candidate set → L2 verification).
4. If a classifier config is provided, run ONNX inference.
5. Return a structured `ScanResult` with match status, distance, and classification scores.

See [docs/architecture.md](docs/architecture.md) for module boundaries and [docs/api.md](docs/api.md) for class-level API reference.

## Quick start

### Requirements

- CMake 3.24+
- C++20 compiler (MSVC 2022, Clang 15+, or GCC 12+)
- [vcpkg](https://github.com/microsoft/vcpkg) with manifest mode

Dependencies are declared in `vcpkg.json`: OpenCV, ONNX Runtime, SQLite, GoogleTest, Google Benchmark.

### Build

**Windows**

```powershell
$env:VCPKG_ROOT = "C:\vcpkg"
cmake --preset windows-default
cmake --build build/default --config Release
ctest --test-dir build/default -C Release --output-on-failure
```

**Linux**

```bash
export VCPKG_ROOT=~/vcpkg
cmake --preset default
cmake --build build/default
ctest --test-dir build/default --output-on-failure
```

**macOS**

```bash
export VCPKG_ROOT=~/vcpkg
cmake --preset macos
cmake --build build/macos
ctest --test-dir build/macos --output-on-failure
```

### CLI examples

```bash
# Fingerprint an image
biopic-cli hash photo.jpg

# Compare two images
biopic-cli compare a.png b.png

# Classify with an ONNX model config
biopic-cli classify photo.jpg --config models/nudity.json

# Full moderation scan (database + optional classifier)
biopic-cli scan photo.jpg --database fingerprints.db --config models/nudity.json

# Manage a fingerprint database
biopic-cli database add photo.jpg --label blocked --database fingerprints.db
biopic-cli database search query.jpg --database fingerprints.db --threshold 50
```

Set `BIOPIC_CLASSIFIER_CONFIG` to avoid passing `--config` on every command.

## Performance

Similarity search uses **LSH bucket indexing** to avoid brute-force L2 over the entire database. The scan path (`find_nearest`) checks well under 1% of records at 100K fingerprints.

| Records | Nearest (scan path) | Brute force |
|---------|---------------------|-------------|
| 10,000 | **~21 µs** | ~1.0 ms |
| 100,000 | **~221 µs** | ~9.5 ms |

At threshold 50, similar-query search verifies ~8% of candidates (for ≥99% recall) and still beats brute force at 100K (~5 ms vs ~8.7 ms).

Full methodology and numbers: [docs/performance.md](docs/performance.md).

Run benchmarks locally:

```bash
cmake --build build/default --target bench_similarity_search
./build/default/bench_similarity_search --benchmark_repetitions=3
```

## C++ API (minimal example)

```cpp
#include "biopic/hasher.hpp"
#include "biopic/image.hpp"
#include "biopic/database/fingerprint_store.hpp"
#include "pipeline/scanner.hpp"

biopic::ImageDecoder decoder;
const auto decoded = decoder.decode_file("photo.jpg");
biopic::ImageView view(decoded->width, decoded->height, decoded->rgb);

auto store = biopic::open_persistent_fingerprint_store("fingerprints.db");
const auto result = biopic::scan(view, store.get());

const biopic::ModerationDecision decision = biopic::scan_decision(result);
```

## C ABI

```c
BiopicEngine* engine = biopic_engine_create();
BiopicFingerprint fingerprint;
biopic_hash_image(engine, data, size, &fingerprint);
biopic_engine_destroy(engine);
```

See [bindings/c/include/biopic.h](bindings/c/include/biopic.h) for ownership rules and error handling.

## Roadmap

| Milestone | Description | Status |
|-----------|-------------|--------|
| 1–5 | Hashing, decoding, classifier, scan pipeline | ✅ Done |
| 6 | SQLite persistent fingerprint storage | ✅ Done |
| 7 | `SimilarityIndex` abstraction, brute-force baseline | ✅ Done |
| 8A | SQLite bucket index, candidate reduction | ✅ Done |
| 8B | LSH neighbor buckets, recall/latency targets | ✅ Done |
| **Polish** | README, docs, CI, GitHub templates | ✅ Done |
| **9** | Policy engine (`ModerationPolicy`) | ✅ Done |
| 10 | CLI subcommand tree | Planned |
| 11 | REST API (`POST /scan`) | Planned |
| 12 | Web dashboard (demo UI) | Planned |
| 13 | Packaging (vcpkg, Homebrew, Docker) | Planned |
| 14 | Vector backends (FAISS, Qdrant, pgvector) | Planned |
| — | Evaluation framework (precision/recall, ROC, labeled datasets) | Planned |

## Documentation

| Document | Description |
|----------|-------------|
| [docs/architecture.md](docs/architecture.md) | Module boundaries and pipeline design |
| [docs/api.md](docs/api.md) | Public API reference |
| [docs/performance.md](docs/performance.md) | Benchmarks and similarity-search tuning |
| [docs/adr/0001-biopichash-v1.md](docs/adr/0001-biopichash-v1.md) | BioPicHash v1 design decision |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Development and PR guidelines |
| [SECURITY.md](SECURITY.md) | Vulnerability reporting |

## Contributing

Contributions are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) and the [Code of Conduct](CODE_OF_CONDUCT.md) before opening a pull request.

## License

Apache-2.0. See [LICENSE](LICENSE).
