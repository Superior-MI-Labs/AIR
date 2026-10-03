# AIR Release Provenance

## AIR 0.11.0

AIR 0.11.0 is the Adaptive Execution Foundation release.

The release source identity is the eventual `v0.11.0` tag target. The hosted
release qualifier records the exact Git commit and tree and generates a
deterministic source archive plus SHA-256 manifest.

### Hosted final qualification

The exact-source hosted gate includes:

- AIR Web 3.3 preflight;
- all 19 CTests;
- isolated installation;
- installed executable/CMake version checks;
- external `find_package(AIR)` consumer;
- installed reference server;
- machine/environment topology agreement;
- semantic-registry resolved/missing truth;
- HTTP generation and Decision;
- malformed native-request rejection;
- detailed timeline and ExecutionGraph R1 reference observation;
- shutdown/restart;
- no leaked installed server process;
- deterministic source/evidence archive checksums.

Prompt 11 defect discovery and correction are retained in
`docs/adaptive-execution/CURRENT.md`.

### Retained machine evidence

Earlier adaptive-execution stages were qualified on the development NVIDIA
machine before its hardware malfunction, including prepared-resource identity,
planner residency transitions, typed token/byte observations, and Qwen CUDA
graph concordance before the final R1 architecture freeze.

Those results remain retained research evidence.

They do not establish final-source post-R1 CUDA qualification.

### Hosted release-candidate gate

The first complete candidate gate passed on
`d7dbe6f8dbdd7a4a867573225bf5da0740bd6b5c` in GitHub Actions run
`37146788192`.

That run generated and uploaded the deterministic source/evidence bundle under
artifact ID `11282591813`.

The final tag target is the documentation-complete successor commit after it
passes the same exact-source workflow. This avoids changing source after
qualification merely to record qualification.

### Explicit 0.11 evidence debt

The 0.11 release manifest records these as not qualified:

- final exact-source post-R1 Qwen CUDA graph concordance;
- final exact-source NVIDIA memory/thermal/power pressure behavior;
- AIR-owned FLUX text-encoder/denoiser/VAE execution;
- FLUX decoded-image parity;
- final interactive human Control Room usability.

### Scope claim

AIR 0.11.0 qualifies a shared adaptive execution architecture across Qwen2 and
the frozen FLUX.2 Klein workflow while keeping production end-to-end model
execution limited to the existing Qwen path.

The release does not claim universal model execution, universal tensor IR, or
arbitrary package-supplied executable semantics.

## AIR 0.10.0

AIR 0.10.0 is the Modular Model Architecture Boundary release.

The authoritative public release is tag `v0.10.0`. Its published source,
qualification evidence, MEF R0 evidence, manifest, and checksums remain frozen
historical artifacts.

AIR 0.10.0 separated qualified Qwen2 model-family interpretation and semantic
tensor binding from backend numerical execution.

## AIR 0.9.12

AIR 0.9.12 was qualified before the public Git repository was created.

Authoritative qualified release identity:

- source fingerprint:
  `587928104d2da182d30903daf00b30d1bd45f3115dcb4896bf3ea3bc70f04fb3`;
- release archive SHA-256:
  `78e463df6c3eb4c8f6f550430fe429579cf08a678bc7cb1d047a702a76763c9f`.

Historical release evidence is never rewritten by later releases.
