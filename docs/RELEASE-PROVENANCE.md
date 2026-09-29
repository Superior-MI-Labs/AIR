# AIR Release Provenance

## AIR 0.10.0

AIR 0.10.0 is the Modular Model Architecture Boundary release.

The release is qualified from the public Git repository and is bound to the
published `v0.10.0` tag. The tag target is the authoritative source commit for
the release. Deterministic source archive and evidence checksums are produced by
the Wave 8 release qualification and published with the GitHub Release.

### Qualification chain

The 0.10.0 release candidate must satisfy:

- Release/CUDA build on WolfCat-Studio;
- all 12 CTests;
- semantic-binding falsification for Reference and CUDA;
- same-machine frozen-v0.9.12 differential non-regression when the historical
  strict 0.001 diagnostic tolerance is not reproducible;
- current manifest schema validation and legacy-schema rejection;
- installed CMake consumer;
- public HTTP, generation, chat, Decision, observability, and AIR Web checks;
- cancellation, shutdown/restart, bounded overload, and resource reclamation;
- frozen MEF R0 `provider.air.http` compatibility for
  `text.generate@1.0.0`.

The external MEF R0 qualification evidence captured during Wave 7 is:

```text
Superior-MI-MEF-R0-Evidence-20260929T184315Z.zip
SHA-256:
ab2fe1ff76b5d4d5227f1b1eb1f570e495c9a8836992753dfd7ba99cfd1ac395
```

Wave 8 reruns the full release candidate after the 0.10.0 version/documentation
freeze so the final release evidence is bound to the exact tagged source.

### Scope claim

AIR 0.10.0 separates qualified model-family interpretation and semantic tensor
binding from backend execution.

Qwen2 remains the only qualified production model architecture.

AIR 0.10.0 does not claim:

- arbitrary GGUF architecture support;
- a universal Neural Model IR;
- arbitrary Builder-authored neural execution;
- multi-family production model support.

## AIR 0.9.12

AIR 0.9.12 was qualified before the public Git repository was created.

Authoritative qualified release identity:

- source fingerprint:
  `587928104d2da182d30903daf00b30d1bd45f3115dcb4896bf3ea3bc70f04fb3`
- release archive SHA-256:
  `78e463df6c3eb4c8f6f550430fe429579cf08a678bc7cb1d047a702a76763c9f`

The GitHub repository is intentionally sanitized. It excludes internal agent
state, machine-local build trees, logs, private research scratch artifacts, and
other unpublished material.

The Git tag `v0.9.12` marks the first sanitized public source snapshot.

The release attachment `AIR-0.9.12-public-release.tar.gz` and its published
checksum remain the authoritative frozen 0.9.12 artifact.

Historical release evidence is not rewritten by later AIR releases.
