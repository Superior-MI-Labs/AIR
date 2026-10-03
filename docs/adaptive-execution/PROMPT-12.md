# Prompt 12 — AIR 0.11.0 Hosted Release Qualification

Status: CLOSED / HOSTED-QUALIFIED / PUBLICATION PENDING
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


## Hosted release-candidate qualification

First complete hosted release-candidate source:

`d7dbe6f8dbdd7a4a867573225bf5da0740bd6b5c`

GitHub Actions:

`37146788192` -> SUCCESS.

Required markers:

- `release_static_contract_checks=PASS`;
- `AIR_WEB_PREFLIGHT=PASS`;
- 19/19 CTests PASS;
- `ADAPTIVE_PREFLIGHT=PASS`;
- `PROMPT11_HOSTED_RC_HARDENING=PASS`;
- installed AIR executables report `0.11.0`;
- installed CMake package reports `0.11.0`;
- `FINAL_HOSTED_RELEASE_CANDIDATE=PASS`;
- `READY_TO_TAG_WITH_LIMITATIONS=v0.11.0`;
- `GPU_FINAL_SOURCE_QUALIFIED=false`;
- `FLUX2_DEVICE_EXECUTION_QUALIFIED=false`;
- `HUMAN_USABILITY_QUALIFIED=false`.

Generated archive checksums for that candidate:

- source:
  `daf383fe322000356c3ed74f8a28c8a353861cfbd0daff3e7c8dae73f9b21f9f`;
- hosted qualification evidence:
  `16bc7d03291545f2477cc880a977f78adb74a140dafb7c56c400190efde85ad7`;
- release manifest:
  `3d9fc7cbdfa71749002a9574f2b0d0ac3f9586cb7294c49822dfb39f76dcae2d`.

GitHub Actions artifact:

- name: `AIR-0.11.0-hosted-release-candidate`;
- artifact ID: `11282591813`;
- uploaded ZIP SHA-256:
  `1e40242c2a35b57721aad222609a1ce2e887e891f9fa6ecd38ce56b10308809e`.

This closure record changes only release documentation. The commit containing
this record must itself pass the identical Prompt 12 hosted release workflow
before it is used as the `v0.11.0` tag target.

Publication is still pending. No tag or GitHub Release is claimed until it
exists publicly.
