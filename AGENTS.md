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

## Frozen Modular Runtime R0 program

The previous architecture program is frozen under:

`docs/modular-runtime/`

It produced the qualified public AIR 0.10.0 release.

Do not reopen that program casually. Treat `v0.10.0` as immutable release
evidence.

## Active Adaptive Execution Substrate R0 program

The active architecture program is:

`docs/adaptive-execution/`

Start with:

1. `docs/adaptive-execution/START-HERE.md`
2. `docs/adaptive-execution/CURRENT.md`
3. `docs/adaptive-execution/PROGRAM.md`
4. `docs/adaptive-execution/WAVE0.md`

Mission:

> evolve AIR into a standalone machine-to-computation compiler/runtime where
> semantic computation, hardware topology, dynamic execution environment,
> physical execution plans, and measured performance evidence are explicit
> separate data.

This program may broaden AIR beyond token-generation workloads, but it must
earn abstractions from concrete implementations. Qwen2 is implementation family
#1; a real multi-component image workflow is the intended second structural
discriminator.

Unknown semantics must fail explicitly. Do not create a model-family engine per
architecture. Do not design a universal Neural IR from theory alone.

## Change discipline

For each implementation wave:

1. refresh the exact repository head;
2. read the active program START-HERE, CURRENT, PROGRAM, and current wave;
3. load only the source/tests needed for the current ownership seam;
4. identify the current owner before editing;
5. add or preserve characterization and regression tests;
6. make the smallest coherent change;
7. delete/retire superseded paths instead of preserving permanent bridges;
8. run the applicable build and qualification gates;
9. preserve negative results and failure evidence;
10. update the active program handoff before context becomes unreliable;
11. do not advance the wave when its exit gate is not satisfied.

For coding-agent context discipline, follow
`docs/adaptive-execution/AGENT-WORKFLOW.md`.

Root-cause fixes are preferred over shims or compatibility branches.

## Frozen releases

The public AIR 0.9.12 and AIR 0.10.0 releases remain immutable evidence for
their respective release claims.

AIR 0.10.0 qualified source:
`3b728a1e45ae3c908aeb859b60cba3c2f5463506`

Do not rewrite the meaning of a frozen tag while developing later architecture.
New work proceeds on explicit branches and earns new release claims through
qualification.
