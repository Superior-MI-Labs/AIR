# AIR Agent Contract

This repository owns AIR engineering behavior.

## Source hierarchy

When claims conflict, use this order:

1. qualified release artifacts and frozen release records for frozen-release claims;
2. current source, tests, machine evidence, and repository status for current implementation facts;
3. current public contracts and support matrices;
4. active architecture and development records;
5. conversation or handoff context;
6. historical narrative.

Do not silently reconcile conflicts. Record them.

## AIR ownership

AIR owns:

- inference-service behavior;
- bounded admission and scheduling;
- prepared backend execution;
- sequence execution state and physical KV behavior;
- reference execution as a numerical correctness oracle;
- CUDA execution;
- AIR-local qualification, verification, metrics, and diagnostics.

AIR does not own:

- Superior MI Builder structural truth;
- MEF provider selection or deployment planning;
- Inference Fabric continuity or response reuse;
- general training or model synthesis;
- a second system graph or capability registry.

## Architecture rules

- Keep one production inference path.
- Keep one scheduler authority.
- Keep one canonical ModelDefinition.
- Prepared state may derive from ModelDefinition but may not duplicate canonical model truth.
- Keep Reference independent enough to remain a useful correctness oracle.
- Keep provider, browser, CLI, HTTP, and diagnostic surfaces thin over AIR runtime authority.
- Fail explicitly on unsupported semantics.
- Do not add hidden fallback behavior.
- Characterize existing behavior before refactoring it.
- Modify existing ownership seams before creating parallel systems.
- Prefer data and explicit contracts over model-family conditionals when evidence justifies the seam.
- Do not generalize from one implementation when the second implementation has not tested the abstraction.

## Modular Runtime R0 program

The active architecture program is documented under:

`docs/modular-runtime/`

Its public endpoint is intended to be AIR 0.10.0, if qualification succeeds.

The goal is narrow:

> separate model-family interpretation and semantic tensor binding from backend execution while preserving AIR's existing runtime, scheduler, serving, state, and public behavior.

This program does not authorize a universal Neural IR, arbitrary GGUF support, Builder-to-AIR compilation, a new scheduler, a new inference engine, or a second model registry.

## Change discipline

For each implementation wave:

1. refresh the exact repository head;
2. read the active modular-runtime plan and baseline;
3. identify the owning seam before editing;
4. add or preserve characterization and regression tests;
5. make the smallest coherent change;
6. run the applicable build and qualification gates;
7. preserve negative results and failure evidence;
8. do not advance the wave when its exit gate is not satisfied.

Root-cause fixes are preferred over shims or compatibility branches.

## Frozen AIR 0.9.12

The public AIR 0.9.12 release remains immutable evidence for that release.

Do not rewrite the meaning of `v0.9.12` while developing later architecture.

Current development may move beyond 0.9.12 on branches or main only through explicit qualification and release work.
