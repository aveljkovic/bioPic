# BioPic Architecture

## Overview

BioPic is a moderation engine that returns structured decisions to host applications. It never deletes content, bans users, or contacts external organizations — the host application owns enforcement.

The current release ships a complete **offline moderation core**: image decode, BioPicHash v1, ONNX classification, SQLite fingerprint storage, LSH-accelerated similarity search, and a unified scan pipeline exposed via C++, C, and CLI.

```
Host Application
       │
       ▼
  ┌─────────┐     ┌──────────────┐     ┌─────────────────┐
  │   CLI   │     │  C++ / C API │     │  Future REST    │
  └────┬────┘     └──────┬───────┘     └────────┬────────┘
       │                 │                       │
       └─────────────────┼───────────────────────┘
                         ▼
                    Scanner
                         │
         ┌───────────────┼───────────────┐
         │               │               │
    ImageDecoder      Hasher      FingerprintStore
         │               │               │
      OpenCV        BioPicHash v1   SimilarityIndex
                                         │
                              PersistentFingerprintStore
                                    (SQLite + buckets)
         │               │               │
         └───────────────┼───────────────┘
                         │
                   IClassifier (ONNX)
                         │
                 ModerationDecision
```

## Component pipeline

The moderation path is intentionally layered. Each stage has a narrow responsibility:

```
ImageDecoder
      ↓
   Hasher
      ↓
FingerprintStore  ←── SimilarityIndex (in-memory or SQLite-backed)
      ↓
  IClassifier
      ↓
   Scanner
      ↓
     CLI
```

| Stage | Responsibility | Key types |
|-------|----------------|-----------|
| **ImageDecoder** | Decode bytes to canonical RGB; enforce size limits | `ImageDecoder`, `ImageView` |
| **Hasher** | Compute BioPicHash v1 perceptual fingerprint | `Hasher`, `Fingerprint` |
| **FingerprintStore** | Persist and query labeled fingerprints | `FingerprintStore`, `FingerprintRecord` |
| **SimilarityIndex** | In-memory similarity search with pluggable indexing | `SimilarityIndex`, `BucketedIndex` |
| **IClassifier** | ONNX-backed image classification | `OnnxClassifier`, `ClassificationResult` |
| **Scanner** | Orchestrate decode → hash → match → classify → decision | `scan()`, `ScanResult` |
| **CLI** | User-facing commands | `apps/biopic-cli` |

## Module map

| Directory | Module | Status |
|-----------|--------|--------|
| `src/image/` | Secure image decode, EXIF orientation | Implemented |
| `src/hash/` | BioPicHash v1, encoding, distance | Implemented |
| `src/index/` | SimilarityIndex, LSH buckets, verification | Implemented |
| `src/database/` | FingerprintStore, SQLite persistence | Implemented |
| `src/ai/` | ONNX Runtime classifier, preprocessing | Implemented |
| `src/pipeline/` | Scanner orchestration | Implemented |
| `apps/biopic-cli/` | Command-line interface | Implemented |
| `bindings/c/` | Stable C ABI | Implemented |
| `api/openapi.yaml` | Future REST contract | Spec only |

## Scan pipeline behavior

When `scan()` runs on an image:

1. **Decode** — Detect format; reject malformed input; apply EXIF orientation; produce 8-bit RGB.
2. **Fingerprint** — Compute BioPicHash v1 (144 bytes, versioned).
3. **Database lookup** (optional) — If a `FingerprintStore` is provided:
   - Check bloom filter for exact match candidates.
   - Run `find_nearest()` through the LSH index (scan path, <1% candidates at scale).
   - Set `MatchStatus` to `ExactMatch` or `SimilarMatch` when within threshold.
4. **Classification** (optional) — If a classifier config path is set, load ONNX model and run inference.
5. **Policy** — `ModerationPolicy` maps detection signals to `Allow`, `Review`, or `Block`.
6. **Decision** — CLI and integrations consume `PolicyEvaluation` (decision + reason).

Hash enrollment requires an explicit `database add` (or API call). Classifier positives do not auto-enroll.

## Policy engine (Milestone 9)

Detection and decision are separated. The scanner produces a `ScanResult`; the policy engine decides:

| Signal | Default decision |
|--------|------------------|
| Exact fingerprint match | BLOCK |
| Classifier confidence ≥ 0.97 | BLOCK |
| Classifier confidence 0.70–0.97 | REVIEW |
| Similar fingerprint match | REVIEW |
| No match | ALLOW |

Configure thresholds via `PolicyConfig`. When multiple signals apply, the most severe decision wins (Block > Review > Allow).

**Planned:** JSON/file-based policy loading and per-label database rules.

## Similarity search architecture

```
Fingerprint
      │
      ├── find_nearest (scan path)
      │        12 exact band hashes + prefix bucket
      │        → candidate union → L2 verification
      │
      └── find_similar / query (threshold search)
               12 packed-nibble LSH tables
               → ±nibble neighbor buckets
               → candidate union → L2 verification
```

Backends implement the same `SimilarityIndex` interface:

```
SimilarityIndex
├── BruteForceIndex      (O(n) baseline)
├── BucketedIndex        (default — LSH + exact bands)
└── [Future] FaissIndex, QdrantIndex, PgVectorIndex
```

The scanner and `FingerprintStore` never change when swapping backends.

## BioPicHash v1 layout

- Feature grid: 12×12 normalized multi-scale sampling
- Output grid: 6×6 with 4 directional channels
- Flattening order: **channel-major** (`index = c×36 + y×6 + x`)
- Encoding prefix: `biopic:v1:`

See [adr/0001-biopichash-v1.md](adr/0001-biopichash-v1.md).

## SQLite schema

Fingerprints are stored in `fingerprints` (id, fingerprint blob, label, created_at). LSH bucket keys are stored in `fingerprint_buckets` (fingerprint_id, band_index, bucket_key) with an index on `(band_index, bucket_key)` for candidate lookup.

## Integration surfaces

| Surface | Status |
|---------|--------|
| C++ library | Implemented |
| C ABI (`biopic_c`) | Implemented |
| CLI | Implemented |
| REST | OpenAPI contract only |
| gRPC | Not started |
| Python bindings | Not started |

## Security principles

- All image bytes treated as hostile input
- Checked arithmetic and allocation limits in decode path
- Parameterized SQL in SQLite store
- Bloom filter for fast exact-match rejection
- Redacted logs: no raw image bytes or full fingerprints in normal logs

## Build layout

```
include/biopic/     Public C++ headers
src/                Core implementation
bindings/c/         C ABI
apps/biopic-cli/    Command-line tool
tests/              Unit and integration tests
benchmarks/         Performance benchmarks
docs/               Architecture, API, performance notes
cmake/triplets/     vcpkg release triplets
```

## CLI structure

The CLI uses a subcommand tree:

```
biopic
├── hash
├── compare
├── scan [--json]
├── evaluate
├── benchmark
├── database
│   ├── add
│   ├── search
│   ├── stats
│   └── vacuum
├── model
│   ├── info
│   ├── verify
│   └── list
├── config
├── doctor
└── version
```

The legacy `classify` command remains available for backward compatibility.

`biopic scan --json` emits machine-readable output. Exit codes: **0** = ALLOW, **1** = REVIEW, **2** = BLOCK, **10** = error.

`biopic evaluate DATASET/` accepts a `manifest.csv` (`path,label`) or `allow/`, `review/`, and `block/` subdirectories.

`biopic doctor` validates the runtime: ONNX Runtime, SQLite, fingerprint pipeline, optional classifier config, and optional database.
