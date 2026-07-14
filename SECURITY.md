# Security Policy

## Reporting a Vulnerability

If you discover a security issue in BioPic, please report it privately.

1. Do **not** open a public GitHub issue for exploitable vulnerabilities.
2. Email the maintainers with a description, reproduction steps, and impact assessment.
3. Allow reasonable time for a fix before public disclosure.

## Scope

BioPic treats all image bytes as hostile input. Report issues related to:

- Memory safety in decoding, fingerprint parsing, and ONNX tensor handling
- Denial of service via decompression bombs or oversized allocations
- SQLite injection or path traversal in database operations
- Information disclosure through logs or error responses
- Authentication and tenant isolation in future REST/gRPC services

## In scope with caveats

- **ONNX model adversarial robustness** — report separately; model quality is deployment-specific
- **Similarity threshold tuning** — not a vulnerability; calibrate per deployment

## Out of scope

- Host-application enforcement logic (BioPic returns decisions; hosts enforce)
- REST/gRPC service attack surface (no server binary ships yet)

## Safe harbor

Good-faith security research that avoids privacy violations, service disruption, and data destruction is appreciated.
