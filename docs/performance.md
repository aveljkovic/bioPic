# BioPic Performance Baseline

This document records similarity-search performance for **Milestone 7** (`BruteForceIndex`). Use it as a baseline when evaluating optimized backends in Milestone 8 (SQLite-assisted indexing) and later vector engines (Qdrant, FAISS, pgvector).

## Architecture (Milestone 7)

```
                 Image
                   |
              Image Decoder
                   |
              Fingerprinter
                   |
             Fingerprint
                   |
          +--------+--------+
          |                 |
 SimilarityIndex       Classifier
 (BruteForceIndex)          |
          |                 |
          +--------+--------+
                   |
             ModerationDecision
                   |
          SQLite Storage (persistence)
```

The scanner performs one `find_nearest()` pass per image when a database is configured. Similar-match detection reuses that result and checks the configured threshold — no second full scan.

## Benchmark setup

| Item | Value |
|------|-------|
| Date | 2026-07-13 |
| Platform | Windows 10, x64 |
| CPU | 12 logical cores @ ~2.9 GHz |
| Build | Release (`cmake --preset windows-default`) |
| Backend | `BruteForceIndex` (O(n) linear scan) |
| Fingerprint size | 144 bytes (BioPicHash v1) |
| Metric | L2 (raw bytes, `use_normalized = false`) |
| Tool | `bench_similarity_search` (Google Benchmark) |
| Repetitions | 3 per configuration |

Run locally:

```powershell
cmake --build build/default --config Release --target bench_similarity_search
build/default/benchmarks/Release/bench_similarity_search.exe --benchmark_repetitions=3
```

### Methodology notes

- **Nearest-neighbor** benchmarks use a query fingerprint that does **not** exactly match any stored record, forcing a full-database distance scan. This matches the common scan path for unknown images.
- **Similar-query** benchmarks use `threshold = 50.0` and return all matches within threshold, sorted by distance.
- Results below use the **CPU mean** from Google Benchmark after 3 repetitions.

## BruteForceIndex — nearest neighbor (`find_nearest`)

Used by `biopic scan` when a database is attached.

| Records | Mean latency | Queries/sec | Scaling |
|---------|--------------|-------------|---------|
| 100 | **7.5 µs** | ~134,000 | — |
| 1,000 | **75 µs** | ~13,300 | ~10× |
| 10,000 | **730 µs** | ~1,370 | ~10× |

Complexity: **O(n)** — latency grows linearly with database size.

### Extrapolated scan cost (nearest only)

| Fingerprints | Estimated latency |
|--------------|-------------------|
| 10,000 | ~0.7 ms |
| 100,000 | ~7 ms |
| 1,000,000 | ~70 ms |
| 10,000,000 | ~700 ms |
| 100,000,000 | ~7 s (unusable) |

## BruteForceIndex — similar query (`query` with threshold)

Used by `database search` and similar-match logic when multiple candidates are needed.

| Records | Mean latency | Queries/sec | Scaling |
|---------|--------------|-------------|---------|
| 100 | **6.6 µs** | ~152,000 | — |
| 1,000 | **67 µs** | ~14,800 | ~10× |
| 10,000 | **679 µs** | ~1,470 | ~10× |

Similar-query cost is comparable to nearest-neighbor at the same scale because both perform a full linear pass over stored fingerprints.

## Performance tests

`test_similarity_index_performance` asserts that 10,000-record nearest and similar queries complete within **500 ms** on CI/dev hardware (conservative smoke test, not a tight SLA).

## Planned backends (Milestone 8+)

```
SimilarityIndex
        |
        +---- BruteForceIndex     (Milestone 7 — baseline above)
        |
        +---- SQLiteIndex         (Milestone 8A — prefix buckets, bloom filters)
        |
        +---- QdrantIndex         (Milestone 8B — external vector engine)
        |
        +---- FAISSIndex          (Milestone 8B — embedded ANN)
```

**Milestone 8A goal:** reduce candidate set before exact distance verification:

```
Fingerprint → bucket key → candidate set (thousands) → exact L2 verification
```

Target: keep scan latency flat or sub-linear as fingerprint count grows into millions, without introducing a separate server process.

## Revision history

| Milestone | Backend | 10K nearest | Notes |
|-----------|---------|-------------|-------|
| 7 | BruteForceIndex | ~730 µs | Initial baseline; linear O(n) |

Update this table when Milestone 8A/8B backends land.
