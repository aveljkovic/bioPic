# Contributing to BioPic

Thank you for your interest in contributing. BioPic is designed with clear abstraction boundaries — changes to similarity search, storage, or classification should not require modifying unrelated components.

Please read the [Code of Conduct](CODE_OF_CONDUCT.md) before participating.

## Development setup

1. Install **CMake 3.24+**, a **C++20** compiler, and **[vcpkg](https://github.com/microsoft/vcpkg)**.
2. Set `VCPKG_ROOT` to your vcpkg installation.
3. Configure with a CMake preset:

   | Platform | Preset |
   |----------|--------|
   | Linux | `default` |
   | macOS | `macos` |
   | Windows | `windows-default` |

4. Build and test:

   ```bash
   cmake --preset default
   cmake --build build/default
   ctest --test-dir build/default --output-on-failure
   ```

## Project structure

```
Hasher → FingerprintStore → SimilarityIndex → Scanner → CLI
```

| If you change… | Also update… |
|----------------|--------------|
| Hash algorithm or parameters | Golden tests, ADR, version constant |
| SimilarityIndex backend | `test_bucketed_index`, `docs/performance.md` |
| SQLite schema | `test_sqlite_fingerprint_store`, migration notes |
| Scanner behavior | `test_scanner`, CLI integration tests |
| Public API | `docs/api.md`, README |

## Code style

- Format C++ with **clang-format** (see `.clang-format`).
- Match existing naming, module boundaries, and header layout under `include/biopic/`.
- Keep hash mathematics in BioPic-owned source files under `src/hash/`.
- Prefer minimal, focused diffs — do not refactor unrelated code in the same PR.

## Tests

BioPic has **27** CTest targets covering hashing, classification, similarity search, SQLite persistence, scanner, CLI, and C ABI.

```bash
ctest --test-dir build/default --output-on-failure
```

Guidelines:

- Golden-vector expected values must only change when `HashParameters` version changes.
- Similarity index changes must preserve or improve recall tests in `test_bucketed_index`.
- Add regression tests for every bug fix.

## Benchmarks

```bash
cmake --build build/default --target bench_hash bench_similarity_search
./build/default/bench_hash
./build/default/bench_similarity_search --benchmark_repetitions=3
```

Include benchmark numbers in PRs that affect performance. See [docs/performance.md](docs/performance.md).

## Pull requests

1. Fork and create a feature branch from `main`.
2. Make your changes with tests.
3. Fill out the [pull request template](.github/pull_request_template.md).
4. Ensure CI passes (Windows, Linux, macOS).

Do **not**:

- Commit model weights without license review.
- Commit secrets, `.env` files, or local database artifacts.
- Break the `SimilarityIndex` / `FingerprintStore` interfaces without an ADR and migration plan.

## Architecture guidelines

- **SimilarityIndex** — pure in-memory search; no I/O. New backends (FAISS, Qdrant) implement this interface.
- **FingerprintStore** — persistence + query; delegates search to `SimilarityIndex`.
- **Scanner** — orchestration only; no direct SQLite or index logic.
- **CLI** — thin wrapper over library APIs.

## Documentation

Update documentation when behavior changes:

- [README.md](README.md) — overview, quick start, roadmap
- [docs/architecture.md](docs/architecture.md) — design and module map
- [docs/api.md](docs/api.md) — public API reference
- [docs/performance.md](docs/performance.md) — benchmarks

## Questions

Open a [GitHub Discussion](https://github.com/aveljkovic/bioPic/discussions) or a feature-request issue for design questions before large changes.

## License

By contributing, you agree that your contributions will be licensed under the [Apache License 2.0](LICENSE).
