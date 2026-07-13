# BioPic Performance Baseline

This document records similarity-search performance for BioPic. Use it when evaluating optimized backends.

## Architecture (Milestone 8)

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
 (BucketedIndex)            |
          |                 |
          +--------+--------+
                   |
             ModerationDecision
                   |
          SQLite Storage + bucket index
```

Candidate reduction flow:

```
Fingerprint → band/prefix bucket keys → candidate set → exact L2 verification
```

The scanner performs one `find_nearest()` pass per image when a database is configured.

## Benchmark setup

| Item | Value |
|------|-------|
| Date | 2026-07-13 |
| Platform | Windows 10, x64 |
| CPU | 12 logical cores @ ~2.9 GHz |
| Build | Release (`cmake --preset windows-default`) |
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

- **Nearest-neighbor** benchmarks use a query fingerprint that does **not** exactly match any stored record.
- **BucketedIndex** uses 12 byte bands + prefix hash (FNV, 65536 buckets), then L2 verification on the candidate union.
- **`find_nearest`** (scan path) uses bucket candidate reduction. **`query` / `find_similar`** verify against the full store for accuracy until Milestone 8B LSH.
- **BruteForceIndex** scans every stored fingerprint (Milestone 7 baseline).
- Results below use the **CPU mean** from Google Benchmark after 3 repetitions.

## Nearest neighbor (`find_nearest`) — used by `biopic scan`

| Records | BruteForce | BucketedIndex | Speedup |
|---------|------------|---------------|---------|
| 100 | **6.8 µs** | **0.65 µs** | ~10× |
| 1,000 | **62.8 µs** | **1.2 µs** | ~52× |
| 10,000 | **642 µs** | **9.6 µs** | ~67× |
| 100,000 | **7.75 ms** | **327 µs** | ~24× |

Complexity:

- **BruteForceIndex:** O(n) — linear with database size
- **BucketedIndex:** O(c) — proportional to candidate set size (typically hundreds, not millions)

### Extrapolated scan cost (nearest, BucketedIndex)

| Fingerprints | BruteForce (est.) | BucketedIndex (measured trend) |
|--------------|-------------------|--------------------------------|
| 1,000,000 | ~70 ms | ~3 ms (projected) |
| 10,000,000 | ~700 ms | ~30 ms (projected) |

## Similar query (`query` with threshold) — BruteForce baseline

| Records | Mean latency |
|---------|--------------|
| 100 | **6.6 µs** |
| 1,000 | **62 µs** |
| 10,000 | **660 µs** |
| 100,000 | **6.93 ms** |

## Performance tests

- `test_similarity_index_performance` — 10,000-record smoke test (&lt;500 ms)
- `test_bucketed_index` — correctness parity with BruteForceIndex

## Backend roadmap (Milestone 8+)

```
SimilarityIndex
        |
        +---- BruteForceIndex        (Milestone 7 — O(n) baseline)
        |
        +---- BucketedIndex          (Milestone 8A — in-memory bucket index)
        |
        +---- SQLite bucket index    (Milestone 8A — fingerprint_buckets table)
        |
        +---- LSH index              (Milestone 8B — multi-level / neighbor buckets)
        |
        +---- FaissIndex             (Milestone 8C — optional vector backend)
        |
        +---- QdrantIndex            (Milestone 8C)
        |
        +---- PgVectorIndex            (Milestone 8C)
```

**Milestone 8A (current):** SQLite stays the default persistence layer. Coarse band + prefix buckets (`fingerprint_buckets` table, bloom filter for exact negatives) reduce candidates before L2 verification. No new servers, works offline.

**Milestone 8B (next):** Expand LSH-style neighbor bucket search and multi-level buckets for higher recall at scale.

**Milestone 8C (later):** Pluggable FAISS/Qdrant/pgvector backends behind the same `SimilarityIndex` interface. Scanner unchanged.

### Candidate counts (nearest neighbor, BucketedIndex)

Typical candidate set size stays far below total fingerprints — L2 runs on hundreds of records, not millions.

| Records | Avg candidates (approx.) | Candidate ratio |
|---------|--------------------------|-----------------|
| 10,000 | ~150–500 | &lt;5% |
| 100,000 | ~500–2,000 | &lt;2% |

Run `bench_similarity_search` to see `candidates` and `candidate_ratio` counters per configuration.

## Revision history

| Milestone | Backend | 10K nearest | 100K nearest | Notes |
|-----------|---------|-------------|--------------|-------|
| 7 | BruteForceIndex | ~730 µs | ~7.5 ms | Linear O(n) baseline |
| 8 | BucketedIndex | ~9.6 µs | ~327 µs | Band + prefix buckets, L2 verify |
