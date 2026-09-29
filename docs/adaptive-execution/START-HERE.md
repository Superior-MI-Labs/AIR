# AIR Adaptive Execution Substrate R0 - Start Here

Status: ACTIVE ARCHITECTURE PROGRAM
Baseline: AIR 0.10.0
Baseline commit: `3b728a1e45ae3c908aeb859b60cba3c2f5463506`
Development branch: `architecture/adaptive-execution-substrate-r0`

## Mission

Evolve AIR from a qualified Qwen2-oriented neural runtime into a standalone,
extensible machine-to-computation compiler/runtime that can admit new model
structures, new execution semantics, and new hardware through explicit typed
contracts without turning AIR into Builder, MEF, or a collection of parallel
model-specific engines.

AIR should eventually answer:

> Given this computation, these immutable resources, this hardware topology,
> this current execution environment, this objective, and this measured
> evidence, what is the best valid physical execution plan AIR can produce and
> execute now?

## Non-negotiable properties

- AIR remains usable standalone.
- AIR remains one runtime, not one engine per model family.
- Model semantics are not inferred or guessed.
- Unknown semantics fail structurally and identify the missing contract.
- Hardware, timing, placement, transfer, memory, and performance observations
  become explicit data rather than hidden heuristics.
- Semantic computation and physical execution remain separate representations.
- Optimization may adapt from evidence; semantic meaning may not.
- High-level modularity may compile away into specialized hot paths.
- Reference execution remains independent enough to function as a correctness
  oracle where applicable.
- AIR does not become Builder structural authority.
- AIR does not absorb MEF provider-selection authority.
- AIR does not absorb Inference Fabric continuity/cache authority.
- No second scheduler, state owner, model registry, or hidden execution path.

## Current qualified foundation

AIR 0.10.0 already established:

```text
GGUF
  -> ModelDefinition
  -> Qwen2 Architecture Adapter
  -> PreparedModelSemantics
  -> Reference / CUDA
  -> InferenceService
  -> CapacityScheduler / MicrobatchScheduler
  -> serving / evidence
```

The important 0.10.0 result is that Reference and CUDA execution consume
semantic tensor bindings rather than reconstructing Qwen2/GGUF source names.

The important limitation is that `PreparedModelSemantics` and `ModelConfig`
are still transformer-shaped and validation still supports only Qwen2.

## New target shape

The program explores this direction:

```text
Model / Workflow Package
          |
          v
Architecture / Workflow Import
          |
          v
Semantic Program
"what must happen"
          |
          +----------------------+
          |                      |
          v                      v
Immutable Resources      Hardware Topology Snapshot
                                +
                         Execution Environment
                                +
                         Measured Evidence
          |                      |
          +-----------+----------+
                      v
              Execution Compiler
                      |
                      v
               Execution Graph
"how this machine will do it"
                      |
                      v
        CPU / CUDA / future devices
```

The exact IR names and shapes are not frozen yet. They must be earned from
multiple genuinely different implementations.

## Second-implementation rule

Do not design a universal Neural IR from Qwen2 alone.

Qwen2 is implementation family #1.

The intended second discriminator is a real multi-component image diffusion
workflow because it introduces:

- multiple model components;
- typed intermediate latent values;
- iterative execution;
- schedulers/solvers;
- component loading/offloading;
- different memory lifetime patterns;
- non-token inputs/outputs.

Only abstractions that survive both Qwen2 and the second implementation should
be candidates for AIR-wide semantic IR.

## Program spine

Read in this order:

1. `AGENTS.md`
2. this file
3. `docs/adaptive-execution/CURRENT.md`
4. `docs/adaptive-execution/PROGRAM.md`
5. the current wave document when created
6. only the source/tests needed for the current question

Supporting records:

- `BASELINE.md` - exact 0.10.0 starting state and refactor seams
- `DATA-MODEL.md` - candidate information/computation/hardware representations
- `GUI.md` - human-facing observability and configuration design
- `QUESTION-BANK.md` - unresolved design/edge-case questions
- `AGENT-WORKFLOW.md` - context-window and coding-agent discipline

## Stop rule

If a future agent cannot state:

- the active wave;
- the owning authority for each proposed change;
- what current evidence supports the abstraction;
- what existing path would be replaced rather than duplicated;
- what test or experiment will falsify the proposal;

then it is not ready to implement the change.
