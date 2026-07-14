# BioPic Documentation

| Document | Description |
|----------|-------------|
| [architecture.md](architecture.md) | System design, module map, pipeline behavior |
| [api.md](api.md) | Public C++ API reference |
| [performance.md](performance.md) | Similarity search benchmarks and tuning |
| [adr/0001-biopichash-v1.md](adr/0001-biopichash-v1.md) | BioPicHash v1 algorithm decision record |

## Component overview

```
Hasher
  ↓
FingerprintStore  (uses SimilarityIndex internally)
  ↓
Scanner           (orchestrates Hasher + Store + Classifier)
  ↓
CLI
```

Start with [architecture.md](architecture.md) for the big picture, then [api.md](api.md) when integrating.
