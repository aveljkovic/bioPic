# Contributing to BioPic

## Development setup

1. Install CMake 3.24+, a C++20 compiler, and vcpkg.
2. Set `VCPKG_ROOT` to your vcpkg installation.
3. Configure with `cmake --preset default` (or `windows-msvc` on Windows).
4. Build with `cmake --build <binary-dir>`.

## Code style

- Format C++ with `clang-format` (see `.clang-format`).
- Match existing naming and module boundaries.
- Keep hash mathematics in BioPic-owned source files.

## Tests

Run `ctest --test-dir <binary-dir> --output-on-failure` after building with `BIOPIC_BUILD_TESTS=ON`.

Golden-vector expected values must only change when `HashParameters` version changes.

## Pull requests

- Describe the moderation or hashing behavior affected.
- Include exact build and test commands run locally.
- Do not commit model weights without license review.
- Do not add placeholder runtime implementations for ONNX, PostgreSQL, or moderation orchestration. Future components must remain abstract interfaces or fail explicitly until fully implemented.
