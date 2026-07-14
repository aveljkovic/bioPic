## Summary

<!-- What does this PR change and why? -->

## Type of change

- [ ] Bug fix
- [ ] New feature
- [ ] Performance improvement
- [ ] Documentation
- [ ] Refactoring
- [ ] CI / build

## Components affected

- [ ] Hasher / fingerprinting
- [ ] Image decoding
- [ ] SimilarityIndex / LSH
- [ ] FingerprintStore / SQLite
- [ ] Classifier / ONNX
- [ ] Scanner / pipeline
- [ ] CLI
- [ ] C ABI
- [ ] Tests / benchmarks

## Test plan

<!-- Commands run locally. Example: -->

```bash
cmake --preset default
cmake --build build/default
ctest --test-dir build/default --output-on-failure
```

- [ ] All existing tests pass
- [ ] New tests added (if applicable)
- [ ] Benchmarks run (if performance-related)

## Checklist

- [ ] Code follows existing module boundaries and naming conventions
- [ ] Golden-vector or hash outputs unchanged (unless `HashParameters` version changed)
- [ ] No secrets, model weights without license review, or `.env` files committed
- [ ] Documentation updated (README, `docs/`, or API comments) if behavior changed

## Breaking changes

<!-- List any API or CLI breaking changes, or write "None". -->

None
