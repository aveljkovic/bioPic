# BioPic

BioPic is an open-source image moderation engine for third-party applications. It combines a custom perceptual fingerprint (BioPicHash) with optional future ONNX classification and database-backed hash matching. Enforcement remains the responsibility of the host application.

## Milestone 1 — implemented

This release provides a **hashing and fingerprinting library** only:

- **BioPicHash version 1** — 144-byte perceptual fingerprint
- **Image decoding** — secure decode to canonical 8-bit RGB via OpenCV
- **Fingerprint serialization** — raw, hexadecimal, and Base64 with version prefix
- **Distance comparison** — L1, L2, and L2-squared metrics
- **CLI** — `biopic hash` and `biopic compare`
- **C++ API** — `biopic::Hasher`, `biopic::ImageDecoder`, `biopic::Fingerprint`, etc.
- **C ABI** — `biopic_c` shared library with stable C bindings
- Unit, golden, determinism, and transformation-distance tests
- Hashing benchmarks

## Milestone 1 — not implemented

The following are **not available** in this release. No runtime code exists for them:

- Nudity classification (`INudityClassifier` is an abstract interface only)
- PostgreSQL / pgvector hash storage (`IHashStore` is an abstract interface only)
- Moderation pipeline orchestration (`ModerationEngine` is not compiled)
- REST service, gRPC, Docker deployment, client SDKs
- Database migrations and hash enrollment APIs

`api/openapi.yaml` describes a future REST contract; it is not served by any binary in this milestone.

## Requirements

- CMake 3.24+
- C++20 compiler (MSVC 2022, Clang 15+, or GCC 12+)
- [vcpkg](https://github.com/microsoft/vcpkg) with manifest mode

### Dependencies (direct)

- **opencv4** — image decode and test image encoding
- **gtest** — unit and golden tests (build-time)
- **benchmark** — performance benchmarks (build-time)

## Build (Windows)

```powershell
$env:VCPKG_ROOT = "C:\vcpkg"
cmake --preset windows-msvc
cmake --build build/msvc --config Release
ctest --test-dir build/msvc -C Release --output-on-failure
```

## Build (Linux/macOS)

```bash
export VCPKG_ROOT=~/vcpkg
cmake --preset default
cmake --build build/default
ctest --test-dir build/default --output-on-failure
```

## CLI usage

```bash
biopic-cli hash path/to/image.jpg
biopic-cli compare image_a.png image_b.png
```

Example output from `hash`:

```
algorithm: biopic
version: 1
sha256_rgb: ...
hex: biopic:v1:...
base64: biopic:v1:...
```

## C ABI

```c
BiopicEngine* engine = biopic_engine_create();
BiopicFingerprint fingerprint;
biopic_hash_image(engine, data, size, &fingerprint);
biopic_engine_destroy(engine);
```

See `bindings/c/include/biopic.h` for ownership rules and error handling.

## SHA-256 scope

`sha256_rgb` is computed over canonical 8-bit RGB row-major bytes (`width * height * 3`).

## Hash distance thresholds

Production near-match thresholds are configuration values and must be calibrated against labeled validation data. No universal threshold is hardcoded.

## Documentation

- `docs/architecture.md`
- `docs/adr/0001-biopichash-v1.md`

## License

Apache-2.0. See `LICENSE`.
