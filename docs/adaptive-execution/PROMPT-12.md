# Prompt 12 — AIR 0.11.0 Hosted Release Qualification

Status: CURRENT
Program: AIR 0.11.0 Adaptive Execution Foundation

## Purpose

Freeze and qualify the exact 0.11 release source using every reproducible gate
available without the failed development laptop.

This prompt does not convert unavailable hardware or human evidence into a
passing result.

## Release title

AIR 0.11.0 — Adaptive Execution Foundation

## Required hosted gates

1. exact release version `0.11.0`;
2. clean branch/worktree and unchanged HEAD;
3. Web 3.3 preflight;
4. all 19 CTests;
5. Prompt 11 installed-product hardening;
6. installed executable version surfaces;
7. installed CMake package version;
8. deterministic source archive;
9. checksummed hosted evidence archive;
10. release manifest bound to exact Git commit/tree;
11. release notes, support matrix, public contracts, architecture, provenance;
12. no hidden promotion of hardware/user evidence debt.

## Allowed final claims

Hosted exact-source qualified:

- CPU/reference installed product;
- public CMake package;
- HTTP generation and Decision;
- Control Room static/installed delivery;
- machine/environment read-only authority;
- detailed timeline;
- ExecutionGraph R1 reference observation;
- semantic registry and MissingSemantic surface;
- restart/reconnect server-side behavior;
- Qwen/FLUX shared structural contracts;
- AIR-owned deterministic FLUX latent-geometry and sigma-schedule semantics.

Retained earlier machine evidence:

- Qwen CUDA adaptive-execution experiments through Prompt 8C;
- identified prepared-resource residency and eviction;
- typed token/byte CUDA observation.

Not final-source qualified:

- post-R1 Qwen CUDA graph replay;
- NVIDIA final-source memory/thermal/power behavior;
- AIR-owned FLUX model-component execution;
- FLUX image parity;
- final interactive human Control Room usability.

## Release rule

A hosted release candidate may report
`FINAL_HOSTED_RELEASE_CANDIDATE=PASS` only when all reproducible hosted gates
pass on one exact source.

It must also report:

`READY_TO_TAG_WITH_LIMITATIONS=v0.11.0`

The limitations are part of the release contract, not a waiver.

## Publication

The qualifier does not create the Git tag or GitHub Release. Publication must
point `v0.11.0` at exactly the hosted-qualified commit and use the generated
manifest/checksums/release notes.

Prompt 12 closes only after the exact release source is hosted-qualified and
the publication state is recorded truthfully.
