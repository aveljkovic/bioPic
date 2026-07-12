# ADR 0001: BioPicHash Version 1

## Status

Accepted (amended 2026-07-12)

## Context

BioPic needs a deterministic perceptual fingerprint to detect reuploads and transformations of known blocked images. The fingerprint must be independent of proprietary systems and suitable for tenant-scoped hash databases with configurable near-match thresholds.

## Decision

Implement BioPicHash version 1 as a 144-byte feature vector with these fixed parameters in `HashParameters`:

| Parameter | Value |
|-----------|-------|
| Feature grid | 12 × 12 |
| Output grid | 6 × 6 |
| Directional channels | 4 (gx+, gx−, gy+, gy−) |
| Box radii (relative to min dimension) | 0.02, 0.05, 0.10 |
| Scale weights | 0.50, 0.30, 0.20 |
| Clip value | 0.20 |
| Epsilon | 1e−12 |
| Relative low-energy ratio | `sqrt(144) × machine_epsilon × 8` |

### Pipeline summary

1. **Intensity projection**: `R + G + B` per pixel (64-bit integral accumulation).
2. **Exact uniformity short-circuit**: if every canonical RGB pixel is identical, return the all-zero fingerprint before any floating-point filtering.
3. **Integral image** with zero border for constant-time rectangle sums.
4. **Fractional rectangle means** via bilinear interpolation on the integral image; boundary rectangles are clipped to image bounds.
5. **12×12 feature map** at normalized cell centers `(i+0.5)/12`, `(j+0.5)/12`.
6. **Central finite-difference gradients** on interior feature cells.
7. **Sign separation** into four non-negative channels.
8. **Bilinear spatial splatting** onto a 6×6 grid per channel.
9. **Relative low-energy guard**: if the raw splat-grid L2 norm is at or below `relative_low_energy_ratio × peak_feature_magnitude`, return the all-zero fingerprint.
10. **Channel-major flattening** to 144 components.
11. **L2 normalize → clip(0.20) → L2 normalize → quantize** with `round(clamp(c/0.20,0,1)×255)`.

The relative low-energy ratio is derived from `sqrt(component_count) × double_machine_epsilon × 8`, scaled by the peak absolute feature-map value. This forms a platform-independent noise floor for degenerate gradients without amplifying floating-point residue during normalization.

### Distance metrics

- L1 (Manhattan): `Σ|a[i] − b[i]|`
- L2 squared: `Σ(a[i] − b[i])²`
- L2 (Euclidean): `√(L2 squared)`

Lower distance means greater similarity. Production thresholds are configuration values, not constants in code.

### Encoding

Fingerprints carry version separately. Text encodings use prefix `biopic:v1:` followed by hex or base64 payload.

## Amendment: cross-platform determinism defect in v0.1.0

Release tag **v0.1.0** shipped a determinism defect: spatially uniform non-zero images (for example pure white or pure red) could produce non-zero fingerprints because bilinear integral-image interpolation introduced tiny floating-point gradients that were amplified by L2 normalization. Windows and macOS produced different noise patterns.

The corrected implementation adds:

1. Integer exact-uniformity detection on canonical RGB input.
2. A relative low-energy guard before normalization.
3. Strict/precise floating-point compilation for the hashing target (`/fp:precise` on MSVC, `-fno-fast-math -ffp-contract=off` on Clang/GCC).

**Golden-vector impact**: `uniform_white_16` and `uniform_red_32` now correctly hash to 144 zero bytes on all platforms. Non-uniform reference images (`gradient_64x48`, `checkerboard_32`) are unchanged.

## Consequences

- Any parameter change that alters output requires a new hash version.
- Golden-vector tests lock the 144-byte output for reference images on all platforms with a single shared expected table.
- Jacobian, QP, and robustness analysis live under `research/robustness` and are not on the production hot path.
- pgvector stores approximate candidates only; final match decisions use exact distance on canonical BYTEA fingerprints.

## Alternatives considered

- External perceptual hash libraries: rejected to keep algorithm ownership and versioning explicit.
- Larger output dimension: deferred; 144 bytes balances retrieval cost and discrimination for v1.
- Platform-specific golden vectors: rejected; uniformity is a mathematical degeneracy and must be handled identically everywhere.
