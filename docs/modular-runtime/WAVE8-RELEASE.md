# AIR Modular Runtime R0 - Wave 8 Release Freeze

## Release endpoint

AIR 0.10.0 — Modular Model Architecture Boundary

Wave 8 begins only after Wave 7 has fully qualified both:

- AIR-local product/runtime behavior;
- frozen MEF R0 `provider.air.http` compatibility.

Both conditions are satisfied.

## Frozen release claim

AIR 0.10.0 separates qualified model-family interpretation and semantic tensor
binding from backend execution.

The qualified flow is:

```text
GGUF
  -> ModelDefinition
  -> Qwen2 Architecture Adapter
  -> PreparedModelSemantics
  -> Reference / CUDA
  -> existing AIR runtime / scheduler / serving
```

Qwen2 remains the sole qualified production model architecture.

The release does not claim universal GGUF execution, a universal Neural Model
IR, arbitrary Builder-authored neural execution, or additional qualified model
families.

## Release-surface changes

Wave 8 is restricted to release/version/documentation/qualification packaging.

The final source freeze includes:

- project version `0.10.0`;
- README release identity;
- support matrix;
- public contracts;
- architecture documentation;
- differential-verification qualification note;
- release provenance;
- release notes;
- public roadmap;
- final release qualification tooling.

No new model architecture, numerical kernel, scheduler, serving path, state
owner, or provider contract may be introduced in Wave 8.

## Final qualification

The canonical final gate is:

```text
scripts/qualify-release-0.10.0.sh
```

It must execute against one clean exact Git HEAD.

The gate performs, in order:

1. release-version and documentation/source-boundary checks;
2. full isolated AIR Wave 7 qualification from source;
3. all 12 CTests;
4. final 0.10.0 executable/CMake version checks;
5. frozen-v0.9.12 same-machine differential non-regression when needed;
6. public HTTP/browser/generation/Decision/stress/overload/lifecycle gates;
7. frozen MEF R0 qualification against that exact newly built AIR prefix;
8. deterministic `git archive` source packaging;
9. SHA-256 generation for source, AIR qualification evidence, and MEF evidence;
10. clean-worktree and unchanged-HEAD verification.

The script does not create or push the Git tag.

## Release artifacts

A successful run produces one release-candidate evidence directory containing:

- `AIR-0.10.0-source.tar.gz`;
- AIR release-candidate qualification evidence;
- frozen MEF R0 evidence;
- `RELEASE-NOTES.md`;
- `RELEASE-PROVENANCE.md`;
- `SUPPORT-MATRIX.md`;
- `RELEASE-SHA256SUMS.txt`;
- `release-manifest.json`;
- exact Git commit/tree/model identities;
- a tag command bound to the exact qualified commit.

## Closure rule

Wave 8 closes when:

1. the final qualifier reports `FINAL_RELEASE_CANDIDATE=PASS`;
2. `v0.10.0` is created at exactly that qualified commit;
3. the tag is pushed;
4. the GitHub Release is published with the 0.10.0 release notes and generated
   release assets/checksums.

The tagged source commit itself is the release identity. The source tree is not
modified merely to write its own commit hash into release documentation.

After the tag/release is published, Branch 1 stops. Further model-family,
Neural Model IR, Builder compiler, model foundry, and experimental neural
architecture work belongs to the later branches that consume this boundary.
