# AIR Adaptive Execution Substrate R0 - Baseline

Baseline release: AIR 0.10.0
Baseline commit: `3b728a1e45ae3c908aeb859b60cba3c2f5463506`

This record exists to prevent the new program from re-inventing structures AIR
already has.

## Existing useful seams

### Model semantics

AIR already has:

- `ModelDefinition` as canonical loaded model truth;
- `ModelArchitectureAdapter`;
- Qwen2 preparation into `PreparedModelSemantics`;
- Reference and CUDA factories consuming prepared semantics.

Pressure:

- `ModelConfig` is transformer-shaped;
- `PreparedModelSemantics` is transformer-shaped;
- prepared validation explicitly supports Qwen2 only;
- tokenizer/chat/generation APIs assume token-centric workloads.

Rule:

Do not replace this path with a parallel generalized path. Refactor it only when
the second implementation proves the required common representation.

### Hardware topology

AIR 0.10.0 already contains `air::HardwareTopology` schema v1 with:

- node kinds: CPU, host memory, accelerator, storage, remote accelerator;
- link kinds: memory access, host-device, peer-device, storage-host,
  storage-device, remote;
- node IDs, backend, architecture, ordinal, NUMA, capacity, available bytes,
  capabilities;
- link measured flag, bandwidth, latency;
- structural validation;
- synthetic laptop and multi-GPU tests.

This is a valuable seed and must be evolved before inventing a second machine
model.

Important pressure discovered at baseline:

- `total_bytes` and device identity look relatively structural;
- `available_bytes` is dynamic environment state;
- measured link bandwidth/latency are empirical evidence, not timeless topology;
- capability strings are currently untyped;
- provenance for observations is not represented;
- snapshot fingerprint semantics need to be characterized;
- current topology is not yet the central input to runtime planning.

Wave 0 must decide whether to split stable topology, dynamic environment, and
measurement evidence while preserving the current public contract or migrating
it explicitly.

### Runtime planning

AIR already has:

- `PlanningInput`;
- `PlanningDecision`;
- `PlanningCandidateTrace`;
- `Planner`;
- `StaticPlanner`;
- `ExecutionPlan`;
- `RuntimeSnapshot`.

Pressure:

- `ExecutionPlan` currently encodes transformer/runtime concepts directly:
  scheduling quantum, KV geometry, quantized linear tactics, attention tactics;
- `RuntimeSnapshot` contains only a small set of dynamic device observations;
- planner input is one `ModelDefinition` + one token-oriented
  `RequestProfile`;
- physical transfers, dependencies, streams, buffer lifetimes, and placement
  are not represented as an explicit execution graph.

Rule:

The future execution compiler should evolve this planning authority. It must
not create a second planner authority.

### Scheduling

AIR already has one:

- capacity-admission authority;
- microbatch scheduler;
- decode/prefill phase behavior;
- request queue/backpressure;
- bounded streaming;
- cancellation and shutdown cleanup.

Pressure:

- work phases are autoregressive-transformer oriented;
- request profiles are token oriented;
- capacity logic is currently expressed around sequence/KV requirements;
- multi-component workflows have different resource lifetime and dependency
  shapes;
- physical scheduling and semantic workload scheduling are not yet separated.

Rule:

Do not add a diffusion scheduler beside the AIR scheduler. Determine which
parts are generic runtime admission/scheduling and which are model/workflow
semantic operations.

### Existing metrics and evidence

AIR already records:

- queue time;
- plan preparation/eviction time;
- prefill time;
- TTFT;
- decode time;
- total time;
- throughput;
- planner candidate evidence;
- queue/active/rejection counts;
- KV/device/prepared-artifact bytes;
- batch counters;
- prefix/sequence-state counters;
- latency percentiles;
- runtime events.

Pressure:

These are useful request-level observations but do not yet provide a complete
causal timeline of CPU preparation, transfers, kernel submission, GPU
execution, synchronization, memory movement, or idle/stall reasons.

Rule:

Extend the existing evidence surface. Do not create an unrelated profiler
database as a second runtime truth.

### Browser

AIR already has a built-in browser command center with Home, Playground,
Decision, Models, Runtime, Diagnostics, Metrics, Setup, and About.

Pressure:

The current browser is request/runtime oriented. The new program needs a
machine/computation/execution mental model that lets a human watch planning and
execution without making the browser an authority.

## Refactor candidates to investigate, not yet approve

1. Split stable hardware topology from dynamic execution environment.
2. Separate measurement records from topology facts.
3. Introduce typed capability/feature descriptors where string capabilities
   become unsafe or ambiguous.
4. Generalize `RequestProfile` only after a non-token workload exists.
5. Factor transformer-specific fields out of the generic portion of
   `ExecutionPlan` only when a second workload demonstrates the distinction.
6. Separate semantic workload phases from physical execution steps.
7. Introduce operation/implementation identity only where current tactics and
   the second implementation justify it.
8. Represent physical dependencies/transfers/synchronization explicitly without
   replacing the one production execution path.
9. Generalize state/lifetime concepts without turning KV-specific state into a
   fake universal abstraction.
10. Move browser data toward typed snapshots/events instead of scraping or
    reconstructing internal state.

## Existing invariants to preserve

- one `ModelDefinition` authority for the existing loaded Qwen model;
- one production `InferenceService`;
- one capacity scheduler;
- one microbatch scheduler;
- Reference remains correctness-oriented;
- CUDA remains optimized implementation;
- prepared state is derived;
- explicit backpressure;
- deterministic cleanup;
- no hidden fallback;
- evidence schemas are versioned;
- browser is not runtime authority;
- MEF remains external provider/deployment authority;
- Builder remains external structural authority.

## Wave 0 characterization targets

Before changing ownership, locate and test:

- every use of `ModelConfig` geometry;
- every direct Qwen2 check;
- every token/sequence assumption;
- every KV-specific scheduling assumption;
- every `ExecutionPlan` consumer;
- every `RuntimeSnapshot` producer/consumer;
- every hardware-topology producer/consumer;
- all timing sources and clock domains;
- all CUDA synchronization points;
- host/device transfer paths;
- allocation/free paths;
- stream usage;
- server/browser metric serialization;
- qualification/manifest dependencies on current fields.
