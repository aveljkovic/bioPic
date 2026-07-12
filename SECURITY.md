# Security Policy

## Reporting a Vulnerability

If you discover a security issue in BioPic, please report it privately.

1. Do not open a public GitHub issue for exploitable vulnerabilities.
2. Email the maintainers with a description, reproduction steps, and impact assessment.
3. Allow reasonable time for a fix before public disclosure.

## Scope

BioPic treats all image bytes as hostile input. Report issues related to:

- Memory safety in decoding and fingerprint parsing
- Denial of service via decompression bombs or oversized allocations
- Authentication and tenant isolation in future REST/gRPC services
- Information disclosure through logs or error responses

## Out of scope for this release

- ONNX model adversarial robustness (report separately once inference is implemented)
- Host-application enforcement logic
- REST/gRPC service attack surface (no server binary ships yet)

## Safe harbor

Good-faith security research that avoids privacy violations, service disruption, and data destruction is appreciated.
