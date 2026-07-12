# ADR 0001: BioPicHash Version 1

## Status

Accepted

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

### Pipeline summary

1. **Intensity projection**: `R + G + B` per pixel (64-bit integral accumulation).
2. **Integral image** with zero border for constant-time rectangle sums.
3. **Fractional rectangle means** via bilinear interpolation on the integral image; boundary rectangles are clipped to image bounds.
4. **12×12 feature map** at normalized cell centers `(i+0.5)/12`, `(j+0.5)/12`.
5. **Central finite-difference gradients** on interior feature cells.
6. **Sign separation** into four non-negative channels.
7. **Bilinear spatial splatting** onto a 6×6 grid per channel.
8. **Channel-major flattening** to 144 components.
9. **L2 normalize → clip(0.20) → L2 normalize → quantize** with `round(clamp(c/0.20,0,1)×255)`.

### Distance metrics

- L1 (Manhattan): `Σ|a[i] − b[i]|`
- L2 squared: `Σ(a[i] − b[i])²`
- L2 (Euclidean): `√(L2 squared)`

Lower distance means greater similarity. Production thresholds are configuration values, not constants in code.

### Encoding

Fingerprints carry version separately. Text encodings use prefix `biopic:v1:` followed by hex or base64 payload.

## Consequences

- Any parameter change that alters output requires a new hash version.
- Golden-vector tests lock the 144-byte output for reference images.
- Jacobian, QP, and robustness analysis live under `research/robustness` and are not on the production hot path.
- pgvector stores approximate candidates only; final match decisions use exact distance on canonical BYTEA fingerprints.

## Alternatives considered

- External perceptual hash libraries: rejected to keep algorithm ownership and versioning explicit.
- Larger output dimension: deferred; 144 bytes balances retrieval cost and discrimination for v1.
