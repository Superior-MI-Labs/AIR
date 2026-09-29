# AIR Adaptive Execution - Candidate Data Model

Status: DESIGN HYPOTHESES
Nothing in this document is a frozen public schema.

The purpose is to keep different kinds of information from collapsing into one
mutable runtime object.

## Information-science rule

AIR must distinguish at least four epistemic classes:

### Fact

An observed or declared property with a defined source.

Examples:

- PCI vendor/device identity;
- CUDA compute capability;
- total physical memory;
- model artifact SHA-256;
- operation semantic version.

### Measurement

A value produced by a measurement procedure in a specific environment.

Examples:

- host-to-device bandwidth;
- kernel duration;
- power draw;
- memory latency;
- thermal steady-state throughput.

A measurement requires provenance and conditions.

### Inference

A conclusion computed from facts/measurements.

Examples:

- "host-to-device transfer is the dominant observed gap";
- "candidate plan likely benefits from double buffering";
- estimated break-even token count.

Inference must identify the evidence it consumed.

### Policy

A human/system objective or permitted choice.

Examples:

- minimize latency;
- cap power;
- prefer deterministic implementations;
- allow/disallow plan autotuning.

Policy is not evidence.

## Existing type evolution

AIR already has `HardwareTopology`, `RuntimeSnapshot`, `ExecutionPlan`,
`PlanningDecision`, and `RequestProfile`.

Prefer evolving/splitting these types over introducing unrelated parallel
representations.

## Candidate: HardwareTopology

Purpose:
Stable-ish physical structure and capabilities of one observed machine.

Candidate content:

```text
HardwareTopology
  identity
  observation provenance
  nodes[]
  links[]
```

Node examples:

- CPU package;
- CPU core group;
- host-memory/NUMA domain;
- accelerator;
- accelerator memory;
- storage;
- remote accelerator endpoint when explicitly supported.

Link examples:

- CPU-memory access;
- host-device;
- peer-device;
- storage-host;
- storage-device;
- remote transport.

Do not embed volatile utilization or free-memory values into long-lived
topology identity without explicit semantics.

## Candidate: ExecutionEnvironmentSnapshot

Purpose:
Dynamic machine state relevant to a planning decision.

Possible values:

- topology identity;
- free/available memory by resource;
- current reservations;
- device utilization;
- CPU load;
- power state;
- temperatures;
- clocks;
- active competing workload indicators;
- runtime/driver versions;
- current backend availability.

This is immutable once captured. A newer observation creates a new snapshot.

## Candidate: MeasurementRecord

```text
MeasurementRecord
  metric identity
  value + unit
  observed interval
  source/tool
  topology identity
  environment identity
  workload identity
  plan identity
  confidence/quality flags
```

Do not treat one measurement as a universal hardware constant.

## Candidate: WorkloadIdentity

The planner needs a stable identity for what is being executed without assuming
that every workload is token generation.

Candidate dimensions:

- semantic program identity;
- immutable resource/package identity;
- input shape/profile;
- requested output shape/profile;
- determinism requirements;
- objective.

The current token-oriented `RequestProfile` remains valid for the existing
generation path until a second workload proves a replacement.

## Candidate: SemanticValue

Do not freeze these yet.

Likely evidence-driven classes include:

- scalar;
- tensor;
- token sequence;
- embedding/conditioning;
- latent;
- image;
- mask.

Video/audio types are deferred until concrete workflows demand their semantic
differences rather than merely tensor storage.

Each value needs:

- semantic type identity;
- element/dtype constraints where meaningful;
- shape/rank constraints;
- mutability/state semantics;
- device-independent meaning.

## Candidate: SemanticOperation

An operation describes meaning, not kernel choice.

Possible fields:

```text
SemanticOperation
  operation identity + version
  typed inputs
  typed outputs
  attributes
  state/effect contract
  determinism contract
```

Do not begin by enumerating a universal operation library.

Derive the first set from:

1. existing Qwen2 computation;
2. the second image workflow.

## Candidate: OperationImplementation

Describes one physical way to implement a semantic operation.

```text
OperationImplementation
  semantic operation reference
  implementation identity + version
  supported backends/hardware
  dtype/shape constraints
  alignment constraints
  workspace model
  deterministic properties
  preparation requirements
  implementation provenance
```

This catalog is AIR-local execution knowledge. It is not Builder's
DefinitionRegistry and not MEF's provider registry.

## Candidate: SemanticProgram

Represents what must happen.

Potential structure:

```text
SemanticProgram
  inputs
  outputs
  immutable resource references
  regions/components
  semantic operations
  explicit data dependencies
  explicit iterative/control regions when justified
  state contracts
```

No device placement, CUDA stream, kernel name, transfer schedule, or measured
latency belongs here.

## Candidate: ExecutionGraph

Represents how one environment will physically execute a semantic program.

Potential physical node types:

- compute;
- transfer;
- allocate;
- release/reuse;
- synchronize/event;
- prepare/compile;
- host operation;
- device operation.

Potential edges:

- data dependency;
- ordering dependency;
- resource dependency.

Possible annotations:

- hardware node;
- stream/queue;
- implementation identity;
- buffer/resource identity;
- expected cost;
- evidence reference.

ExecutionGraph is derived and disposable.

## Candidate: ExecutionObjective

Do not encode a universal scalar "score" too early.

Candidate explicit objectives:

- latency;
- TTFT/first-output;
- sustained throughput;
- concurrency;
- memory ceiling;
- energy;
- power ceiling;
- thermal ceiling;
- deadline;
- balanced profile.

If multiple objectives exist, the trade-off policy must be explicit.

## Candidate: ExecutionObservation

Represents what actually happened.

Possible event/span structure:

```text
ExecutionSpan
  correlation/request/program IDs
  operation/physical-step ID
  resource/device
  queue timestamp
  start timestamp
  end timestamp
  bytes/elements processed
  parent/dependency IDs
  source clock/domain
  optional hardware counters
```

Derived bottleneck labels must not overwrite raw spans.

## Candidate: BottleneckInference

```text
BottleneckInference
  category
  affected interval/operation
  supporting evidence IDs
  confidence
  alternative explanations
```

Candidate categories:

- dependency wait;
- host preparation;
- kernel submission;
- host-device transfer;
- device-host transfer;
- synchronization;
- device memory pressure;
- host memory pressure;
- compute saturation;
- memory-bandwidth saturation;
- insufficient parallelism;
- contention;
- thermal/power throttling;
- unknown.

## Candidate: PlanQualification

A plan is promoted only with evidence bound to:

- exact semantic program identity;
- exact resources;
- topology identity;
- relevant environment/toolchain identity;
- objective;
- measurement methodology.

Stale evidence must fail closed when a changed condition invalidates it.

## Unknown architecture protocol

Desired behavior:

```text
unknown package
    ->
format/package importer
    ->
architecture/workflow description
    ->
required semantic contracts
    |
    +-> all known -> validate/lower
    |
    +-> missing -> structured MissingSemantic result
```

The missing result should identify:

- missing operation/component semantic identity;
- required input/output types;
- package/provenance source;
- why no implementation is legal;
- what extension boundary would need work.

Unknown architecture does not authorize remote code execution or generated
native code.

## Security boundary

A future extension mechanism must separate:

- declarative semantic definitions;
- immutable model/resource artifacts;
- trusted compiled implementations;
- untrusted package metadata.

Loading a model package must not imply executing arbitrary Python/C++/shell
code.

## Builder/MEF relation

AIR data structures describe neural/computational execution and physical
machine execution.

Builder remains owner of system structural composition.

MEF remains owner of choosing/qualifying which provider realizes a high-level
capability.

A future Builder-to-AIR compiler may lower Builder-owned structure into AIR
semantic programs, but AIR remains independently usable without Builder.
