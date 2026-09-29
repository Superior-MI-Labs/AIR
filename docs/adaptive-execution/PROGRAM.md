# AIR Adaptive Execution Substrate R0 - Program

Status: ACTIVE
Baseline release: AIR 0.10.0
Working branch: `architecture/adaptive-execution-substrate-r0`
Candidate public endpoint: AIR 0.11.0, subject to evidence and qualification.

## Endpoint

The R0 endpoint is not "AIR supports every model."

The endpoint is:

> AIR can represent a computation independently from its source artifact
> conventions, represent the physical machine and execution environment as
> inspectable data, compile a valid computation into an explicit physical
> execution graph, execute it through one runtime, measure the result, and
> improve execution strategy through qualification-backed evidence.

The endpoint must be demonstrated by at least two structurally different model
families/workflows. Qwen2 is the retained family. A multi-component image
generation workflow is the intended second discriminator.

Unknown architectures must either lower through known semantic contracts or
fail with a precise unsupported/missing-semantic result. AIR must never guess
what an unknown operator means.

## Core separations

```text
Artifact/package identity
        !=
Semantic computation
        !=
Execution plan
        !=
Hardware topology
        !=
Dynamic machine state
        !=
Measured performance evidence
```

These identities may refer to each other but must never collapse into one
mutable object.

## Candidate architecture layers

### 1. Package and resource layer

Owns immutable identities for weights, configs, processors, tokenizers,
schedulers, adapters, codecs, and other resources needed by a workload.

It does not decide execution placement.

### 2. Semantic computation layer

Describes what must happen independent of CPU/CUDA placement and tactic choice.

The representation must eventually be able to express more than
autoregressive transformers, but no universal operation set is frozen before
the second implementation.

### 3. Hardware topology layer

Represents physical execution resources and links as data:

- CPU packages, cores, threads, SIMD capabilities;
- caches and memory hierarchy;
- NUMA domains where present;
- RAM capacity and measured bandwidth/latency;
- accelerators/devices;
- device memory;
- interconnects such as PCIe/NVLink-like links when observed;
- copy/compute concurrency capabilities;
- driver/runtime/toolchain identity.

Static facts and measured properties must remain distinguishable.

### 4. Execution environment layer

Represents dynamic conditions that can materially change a plan:

- available memory;
- device memory pressure;
- power/thermal state;
- clock behavior;
- competing workloads;
- active reservations;
- device availability;
- runtime/driver changes.

This is observation evidence, not system structural authority.

### 5. Execution compiler

Consumes semantic computation + resources + hardware/environment + objective.

May derive:

- device placement;
- operation implementation choice;
- memory plan;
- buffer lifetime/reuse;
- transfers;
- synchronization;
- stream/queue assignment;
- batching;
- tiling;
- fusion;
- graph capture/replay;
- CPU/GPU overlap;
- precision/tactic selection where semantically valid.

It emits an explicit immutable execution plan/graph.

### 6. Runtime

Executes the chosen plan through AIR's one runtime authority.

The runtime may not silently redesign the semantic computation.

### 7. Evidence and adaptation

Records execution observations with provenance and uses them to compare plans.

Optimization evolution follows:

```text
ACTIVE -> CANDIDATE -> SHADOW -> QUALIFIED -> PROMOTED
```

An adaptive plan may change physical execution. It may not invent semantic
meaning.

## Objectives

Execution objectives must be explicit rather than hidden in heuristics.

Candidate objectives include:

- minimum latency;
- minimum time to first output;
- maximum sustained throughput;
- maximum throughput under a memory ceiling;
- maximum concurrency;
- minimum energy;
- maximum performance under a power or thermal ceiling;
- real-time deadline satisfaction;
- balanced/default.

Multi-objective policy is an explicit configuration problem, not an implicit
ranking.

## Planned waves

### Wave 0 - freeze, census, and observability contract

Goal:
Understand the complete AIR 0.10.0 execution path before refactoring.

Work:

- freeze source/release identities;
- classify every transformer/token/sequence assumption;
- classify every CPU/CUDA/hardware assumption;
- classify scheduler assumptions;
- classify memory/KV-specific assumptions;
- inventory timing and profiling data already available;
- inventory public/browser diagnostics;
- define fact vs measurement vs inference vs policy evidence types;
- define a bounded live-observability contract for the future GUI.

No production execution redesign.

Exit:

- assumption census is complete enough to identify ownership seams;
- proposed new types each have an owning responsibility;
- no parallel runtime introduced.

### Wave 1 - Hardware Topology Snapshot

Goal:
Make physical machine structure explicit, immutable, inspectable data.

Work:

- topology schema;
- CPU/memory/device/link identities;
- static capability observations;
- provenance and snapshot identity;
- Linux/NVIDIA implementation first;
- fixtures for CPU-only, integrated, single-GPU, multi-GPU, NUMA, and partial
  observation.

No performance planner yet.

Exit:

- deterministic snapshot;
- malformed/partial observations are explicit;
- current AIR can run unchanged while consuming no topology policy.

### Wave 2 - Execution Observation

Goal:
Measure where time and resources actually go.

Work:

- timestamps and correlation IDs across admission, CPU preparation, transfers,
  kernel submission, GPU work, synchronization, streaming, and completion;
- operation/phase timing where observable;
- memory traffic and allocation observations when trustworthy;
- power/thermal/clock observations as optional evidence;
- bounded event buffering;
- observer overhead characterization;
- idle/stall cause taxonomy with confidence/source.

Exit:

- an execution can be reconstructed as a timeline;
- observations distinguish measured facts from inferred bottleneck causes;
- instrumentation overhead is bounded and measured.

### Wave 3 - Semantic Operation and Implementation Contracts

Goal:
Separate semantic operation identity from physical implementation identity.

Work:

- characterize existing Qwen2 operations/tactics;
- define the smallest operation/implementation interface supported by evidence;
- define implementation legality constraints;
- define immutable implementation metadata;
- keep current Reference/CUDA math unchanged initially.

Exit:

- planner can ask which physical implementations are legal for an operation
  without knowing source tensor names;
- no speculative universal op catalog.

### Wave 4 - Execution Graph R0

Goal:
Represent physical scheduling explicitly.

The execution graph may include:

- compute nodes;
- transfers;
- allocations/lifetimes;
- synchronization/events;
- device placement;
- stream/queue assignment;
- dependencies.

It is derived execution state, never semantic truth.

Exit:

- existing Qwen2 execution can be represented or faithfully projected into the
  graph without creating a second production path;
- replay/identity is deterministic for the same inputs/environment.

### Wave 5 - Schedule Compiler and Idle-Elimination Experiments

Goal:
Use evidence to improve physical scheduling.

Experiments may include:

- CPU preparation overlap;
- host/device copy overlap;
- pinned/staged transfers;
- CUDA streams/events;
- CUDA Graph capture for stable repeated workflows;
- buffer reuse;
- prefetching;
- batch/tiling choices;
- operation fusion candidates;
- scheduler/execution coordination.

Every optimization requires a baseline, hypothesis, correctness check, and
machine evidence.

Exit:

- at least one retained optimization demonstrates a real bottleneck reduction;
- negative results are recorded;
- scheduler remains one authority.

### Wave 6 - Second Architecture: Multi-Component Image Workflow

Goal:
Use a structurally non-Qwen workload to falsify the abstractions.

Initial strategy:

1. characterize an existing external image pipeline;
2. model its components, resources, intermediates, iteration, and state;
3. run it first as an external research/provider reference if useful;
4. identify which semantics AIR can already represent;
5. introduce only the missing abstractions justified by this implementation.

Expected pressure points:

- text encoder;
- latent values;
- denoising/transformer or UNet component;
- iterative scheduler/solver;
- VAE decode;
- model component residency/offload;
- image output.

Exit:

- Qwen2 and image workflow share real AIR abstractions;
- neither is forced into the other's shape;
- no separate "diffusion engine" owns a competing runtime.

### Wave 7 - Extensible Semantic Boundary

Goal:
Prove AIR can admit a semantic operation or component type not present when
the core was written.

Unknown semantics must produce a structured result such as
`MissingSemanticImplementation`/equivalent, not hidden fallback.

Extension must include:

- semantic definition;
- validation;
- Reference/correctness implementation where meaningful;
- implementation registration/discovery;
- destructive tests;
- provenance/version identity.

Exit:

- new semantics can enter without editing unrelated planner/runtime code;
- extension boundaries do not become arbitrary plugin execution.

### Wave 8 - Evidence-Based Autotuning Laboratory

Goal:
Search physical execution choices without allowing uncontrolled semantic
mutation.

Work:

- candidate generation;
- bounded search spaces;
- replayable experiment identity;
- warmup and statistical measurement rules;
- noise/thermal controls;
- shadow execution;
- promotion gates;
- rollback.

Exit:

- a plan can be promoted only from retained evidence;
- machine/environment binding is explicit;
- stale evidence cannot silently apply to changed hardware/runtime.

### Wave 9 - GUI and Human Control Qualification

Goal:
Make the architecture understandable and usable by a person.

The browser must expose:

- model/workflow structure;
- machine topology;
- current execution plan;
- live timeline;
- CPU/GPU/transfer activity;
- memory residency;
- bottleneck evidence;
- active vs candidate plan;
- objective/policy;
- qualification status;
- explicit unsupported semantics.

The browser remains a surface, never another planner or state owner.

Exit:

- user can explain what AIR is doing and why from the GUI;
- every mutation flows through canonical AIR commands/contracts;
- GUI disconnect cannot affect runtime correctness.

### Wave 10 - Destructive qualification and release

Attack:

- unknown architectures;
- unknown operations;
- malformed topology;
- hotplug/device loss;
- driver/toolchain drift;
- stale measurements;
- low-memory behavior;
- thermal/power drift;
- CPU-only behavior;
- multi-device ambiguity;
- concurrent workloads;
- observer overload;
- GUI reconnect/replay;
- cancellation/shutdown;
- external MEF compatibility.

If qualified, freeze AIR 0.11.0.

## Explicitly deferred

Unless a wave proves otherwise, this program does not authorize:

- cluster-wide distributed training;
- general model training/fine-tuning;
- Builder System IR inside AIR;
- MEF provider selection inside AIR;
- Inference Fabric response reuse inside AIR;
- autonomous source-code generation as a runtime fallback;
- unbounded plugin execution;
- self-modifying semantic definitions;
- a universal graph IR designed only from theory;
- video/audio semantics before concrete implementations justify them;
- Laya/Jev semantics before their computational contracts are explicit.

## Research references

Useful external precedents, not authorities:

- MLIR dialects/interfaces: extensible operations/types without forcing every
  pass to special-case every concrete operation.
- CUDA Graphs: separate workflow definition from repeated execution and reduce
  repeated host submission overhead.
- Modular Diffusers: multi-component workflow blocks, explicit state, loops,
  lazy component loading, and image/video/audio pipeline composition.

AIR should learn from these designs while retaining its own authority model and
qualification discipline.
