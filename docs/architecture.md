# BioPic Architecture

## Overview

BioPic is a moderation engine intended to return structured decisions to host applications. It never deletes content, bans users, or contacts external organizations.

**Milestone 1** ships only the hashing core: image decode, BioPicHash v1, fingerprint encoding, and distance metrics. Classification, database matching, and moderation orchestration are planned but not compiled in this release.

```
[Milestone 1 — implemented]
  Image bytes -> ImageDecoder -> canonical RGB -> Hasher -> Fingerprint -> distance/encoding

[Future milestones — not compiled]
  Host App -> Transport (REST/gRPC/SDK) -> ModerationEngine -> Decision
                                              |
                         +--------------------+--------------------+
                         |                    |                    |
                   ImageDecoder          Hasher (v1)      INudityClassifier
                         |                    |                    |
                   OpenCV decode         BioPicHash         ONNX Runtime
```

## Module boundaries

| Module | Milestone 1 status |
|--------|-------------------|
| `image` | **Implemented** — secure decode, EXIF orientation, canonical RGB, SHA-256 |
| `hash` | **Implemented** — BioPicHash v1, encoding, L1/L2 distance |
| `inference` | **Interface only** — `INudityClassifier` abstract class; no ONNX runtime |
| `storage` | **Interface only** — `IHashStore` abstract class; no PostgreSQL |
| `policy` | **Not compiled** — `ModerationEngine` deferred to a future milestone |
| `server` | **Not started** — REST/gRPC transport |
| `bindings/c` | **Implemented** — stable C ABI over the C++ core |

## Planned moderation pipeline (future)

When classification and storage are implemented, the pipeline will:

1. Receive encoded bytes and optional subject identifier.
2. Enforce decode limits (size, dimensions, pixels, timeout, formats).
3. Detect format from bytes; reject malformed or animated images.
4. Apply EXIF orientation and convert to 8-bit RGB.
5. Compute SHA-256 over canonical RGB bytes.
6. Compute BioPicHash version 1.
7. Search known hashes: exact SHA-256, exact fingerprint, approximate candidates with exact rescoring.
8. If blocked hash match within threshold, return `BLOCK` / `KNOWN_HASH_MATCH`.
9. Otherwise run ONNX classifier when configured.
10. Map classifier scores to `ALLOW`, `REVIEW`, or `BLOCK`.
11. Return structured result; do not retain raw images by default.

Hash enrollment will require authenticated administrative action. Model positives will not auto-enroll.

## BioPicHash v1 layout

- Feature grid: 12×12 normalized multi-scale sampling
- Output grid: 6×6 with 4 directional channels
- Flattening order: **channel-major** (`index = c×36 + y×6 + x`)
- Encoding prefix: `biopic:v1:`

## Integration surfaces

| Surface | Milestone 1 |
|---------|-------------|
| C++ library (hash/decode/distance) | Implemented |
| C ABI | Implemented |
| CLI (`hash`, `compare`) | Implemented |
| REST | OpenAPI contract only; no server binary |
| gRPC | Not started |
| Python bindings | Not started |
| SDKs | Not started |

## Security principles

- Checked arithmetic and allocation limits in decode path
- Parameterized SQL (when storage is implemented)
- API key hashing and scoped access (when REST is implemented)
- Redacted logs: no raw image bytes or full fingerprints in normal logs

## Build layout

```
include/biopic/     Public C++ headers
src/                Core implementation (image, hash)
bindings/c/         C ABI
apps/biopic-cli/    Command-line tool
tests/              Unit and golden tests
benchmarks/         Performance benchmarks
api/openapi.yaml    Future REST contract (not served in M1)
```
