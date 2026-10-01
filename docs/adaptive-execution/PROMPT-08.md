# AIR Adaptive Execution Substrate R0 - Prompt 8

Status: CURRENT
Wave: Second architecture through AIR

## Purpose

Prompt 8 is the first implementation wave driven by the qualified Qwen2 +
FLUX.2 comparison.

It does not attempt to generalize AIR for every current or future model family.

It introduces only the abstractions that are required by both:

1. the already-qualified autoregressive Qwen2 execution path; and
2. the qualified FLUX.2 Klein multi-component iterative image workflow.

The objective is to evolve AIR from a token-shaped physical runtime into a
workload-aware physical execution substrate without creating a second runtime,
planner, scheduler, capacity authority, or resource manager.

Prompt 8 must keep a hard separation between:

```text
semantic meaning
    !=
workload structure
    !=
physical realization
    !=
resource residency
    !=
measured evidence
```

## Frozen evidence entering Prompt 8

### Qwen2

Prompts 1-6 already qualify:

- hardware topology and dynamic environment as separate data;
- execution observation with bounded overhead;
- semantic operation / legal implementation separation;
- ExecutionGraph R0 as immutable derived physical data;
- one scheduler and one capacity authority;
- transition-aware strategy selection;
- optional prepared artifacts;
- measured preparation and eviction behavior;
- resource/capacity-aware strategy transitions.

Prompt 6 specifically proves that physical realization may depend on:

- request horizon;
- prepared-state hot/cold state;
- prepared-state size;
- preparation cost;
- eviction cost;
- available device memory;
- objective.

### FLUX.2 Klein 4B

Prompt 7 qualifies:

- package identity for diffusion model, text encoder, and VAE;
- semantic values:
  - prompt text / conditioning;
  - seeded noise;
  - sigma schedule;
  - iterative latent state;
  - decoded image;
- one latent semantic geometry observed as `[1, 128, 64, 64]`;
- a four-transition sigma path;
- independently meaningful components:
  - text encoder;
  - denoiser;
  - VAE;
- external component staging observations:
  - text encoder `3669 MB`;
  - denoiser `3882 MB`;
  - VAE `160 MB`;
- retained observer evidence with:
  - 26 transition events;
  - 20 residency timeline points;
  - 324 GPU telemetry samples;
  - `11791 MiB` peak device memory;
  - all required components released before the explicit forced-free boundary;
- semantic output identity unchanged under observation.

Exact per-layer asynchronous DynamicVRAM residency remains intentionally
unknown.

## Prompt 8 primary question

What is the smallest AIR-owned workload boundary that lets both Qwen2 and the
qualified FLUX.2 workflow lower into the same physical planning/resource/
execution-observation authorities without forcing either workload into the
other's semantic shape?

## Non-goals

Prompt 8 must not introduce:

- a universal tensor IR;
- a universal neural-operation catalog;
- a generic graph language merely because one could be designed;
- a ComfyUI-compatible workflow engine;
- a Python execution host;
- an image-specific scheduler;
- an image-specific planner;
- an image-specific residency manager;
- a second capacity authority;
- hidden semantics encoded in string names and interpreted by AIR core;
- an assertion that external ComfyUI lifecycle behavior is AIR's future
  lifecycle behavior;
- full end-to-end FLUX.2 execution claims before AIR actually owns and
  qualifies that execution.

## Architecture rule: generalize structure, not unknown meaning

Prompt 7 demonstrated that some structure is reusable while semantic meaning is
workload-specific.

AIR core may generalize facts such as:

- there is a workload kind;
- work occurs in phases;
- a phase has a measurable work-unit interpretation;
- execution may have iterative state;
- physical execution may require identified prepared resources;
- a resource may be resident or nonresident;
- a physical invocation has placement, dependencies, and output behavior;
- observations have phase/work-unit/resource identity.

AIR core must not infer that an arbitrary unknown workload's values mean
"conditioning", "latent", "token", "audio frame", or anything else.

Known semantic adapters own known meaning.

Prompt 9 remains responsible for the unknown-semantics extension protocol.

## Shared authority model

The following remain single authorities:

```text
Model/workload semantic adapter
        |
        v
AIR planning authority
        |
        v
AIR capacity / residency accounting
        |
        v
AIR physical execution graph
        |
        v
AIR runtime
        |
        v
AIR observation / evidence
```

There is no FLUX-specific parallel column.

## Existing Qwen-specific pressure points

Current `include/air/execution.hpp` exposes several token-shaped assumptions:

- `RequestProfile` contains:
  - `prompt_tokens`;
  - `max_output_tokens`;
  - `active_sequences`;
- `RuntimeSnapshot` has:
  - aggregate `resident_kv_bytes`;
  - aggregate `prepared_artifact_bytes`;
- `SchedulingPolicy` owns `prefill_quantum_tokens`;
- `KvPolicy` assumes KV state;
- `PhysicalInvocationKind` contains only prefill/decode forms;
- `PhysicalOutputMode` contains logits/greedy/target-logprobs;
- `ExecutionPayloadKind` contains token/logit payloads;
- `PhysicalInvocationParticipant.work_units` has an implicit token meaning.

Current `include/air/manifest.hpp` additionally assumes:

- token-shaped workload regions;
- expected horizon in tokens;
- token throughput metrics;
- TTFT as the primary first-output latency concept.

These are valid Qwen contracts.

They are not universal AIR contracts.

Prompt 8 must preserve their Qwen meaning while introducing a boundary above
them.

## Smallest implementation packet

Prompt 8 is divided into stages so that every abstraction is falsifiable before
it becomes product authority.

### Stage 8A - characterize and introduce the workload boundary

Goal:

Add the minimum compile-time distinction between the two qualified workload
structures without changing Qwen numerical execution.

Candidate concepts:

```cpp
enum class WorkloadKind {
    autoregressive_tokens,
    iterative_state,
};

enum class WorkUnitKind {
    tokens,
    iterations,
};
```

The exact names may change during implementation review.

Required behavior:

- current Qwen request behavior remains unchanged;
- Qwen's existing `RequestProfile` public contract is characterized before
  migration;
- AIR gains an explicit workload-level request/profile object that can carry
  either:
  - the current autoregressive token profile; or
  - the qualified iterative-state profile;
- no generic map-of-strings becomes semantic authority.

For the iterative FLUX.2 profile, only fields justified by Prompt 7 may enter
the shared planning boundary.

Likely planning dimensions:

- finite iteration count;
- active workload instances;
- known state/value identity supplied by the semantic adapter;
- known prepared resources supplied separately from request shape.

Do not add width/height/dtype/device placement merely because they exist.
Those belong only if a later Prompt 8 experiment proves they materially affect
the AIR boundary being implemented.

### Stage 8B - identified prepared-resource residency

Goal:

Replace aggregate-only optional prepared-state reasoning with identified
resource residency while retaining one resource/capacity authority.

Prompt 6 currently proves one optional Qwen dense prepared artifact.

Prompt 7 proves at least three independently meaningful FLUX.2 components.

The physical runtime therefore needs identified resource state rather than only
one aggregate prepared byte count.

Required properties:

- stable resource identity;
- device-resident byte observation when known;
- residency state;
- workload ownership / provenance;
- no scheduler policy inside the resource record;
- no semantic interpretation of resource contents by generic capacity code;
- total resident prepared bytes are derived from identified resource records,
  not separately authored competing truth.

Migration rule:

The current Qwen dense prepared artifact must lower into the same identified
resource representation.

Do not retain two independently writable sources of truth such as:

```text
aggregate prepared_artifact_bytes
AND
vector of prepared resources
```

If source compatibility temporarily requires an aggregate accessor, it must be
a derived view.

### Stage 8C - explicit work-unit identity in planning and observation

Goal:

Remove the hidden assumption that every `work_units` value means tokens.

Required evolution:

- physical invocation work-unit kind is explicit;
- execution observations carry the same interpretation;
- Qwen reports token work;
- the qualified FLUX.2 denoising loop reports iteration work;
- raw counts remain counts, not converted into invented universal FLOP units.

Prompt 8 must not create a fake cross-workload scalar performance score.

Cross-workload comparison is not currently required.

### Stage 8D - workload-scoped physical invocation vocabulary

Goal:

Let ExecutionGraph describe a known iterative multi-component realization
without making token invocation enums universal.

Two valid implementation directions may be tested:

1. a discriminated workload-specific physical invocation variant; or
2. a small common invocation header plus workload-owned typed payload.

Selection rule:

Choose the design that:

- keeps Qwen compile-time semantics intact;
- allows FLUX.2 phases to be represented without token vocabulary;
- does not require AIR core to understand unknown future semantics;
- serializes deterministically;
- makes unsupported workload/payload combinations fail validation;
- does not create a second graph type or second graph authority.

The current Qwen invocation representation must remain a qualified member of
the resulting model, not be rewritten into an image-shaped abstraction.

### Stage 8E - iterative state and known FLUX.2 semantic adapter

Goal:

Represent only the semantic structure proven by the frozen FLUX.2 oracle.

The known FLUX.2 semantic adapter may own typed concepts for:

- conditioning;
- seeded noise;
- finite sigma schedule;
- iterative latent state;
- decoded image output.

AIR generic physical code may reference stable value identities and lifetimes.

It must not reinterpret the values.

Required semantic sequence:

```text
encode conditioning
        ->
derive negative conditioning
        ->
initialize latent
        ->
realize seeded noise
        ->
derive finite schedule
        ->
iterative sample / state transform
        ->
decode latent
```

The exact tensor storage representation remains separate from this semantic
sequence.

### Stage 8F - component/resource physical plan

Goal:

Lower the known FLUX.2 semantic workflow into one AIR physical plan authority.

Minimum known components:

- text encoder;
- denoiser;
- VAE.

The plan must be able to state:

- which identified component/resource a compute region requires;
- which hardware resource is selected;
- ordering/dependencies;
- iterative denoiser work count;
- output value identity;
- resource residency expectations required by the plan.

The plan must not fabricate:

- exact asynchronous copy completion;
- exact per-layer residency;
- transfer overlap;
- stream assignment;
- eviction cost;

unless AIR itself measures or owns those facts.

The external ComfyUI DynamicVRAM policy is evidence about one oracle
realization, not the AIR plan.

### Stage 8G - ExecutionGraph R1 projection

Goal:

Project both qualified Qwen physical work and the FLUX.2 physical plan into one
ExecutionGraph authority.

Likely required graph evolution:

- explicit workload kind;
- explicit work-unit kind;
- component/prepared-resource identity for compute regions;
- workload-scoped payload/value identity;
- iterative region representation or deterministic repeated node projection;
- non-token output mode.

Do not add allocation/transfer/synchronization nodes unless AIR has an actual
physical fact to represent.

An absent node is preferable to a fabricated physical event.

Exit evidence must include:

- deterministic graph identity;
- Qwen graph nonregression;
- FLUX.2 graph validation;
- illegal cross-workload payload/invocation combinations rejected;
- graph remains immutable derived data;
- graph remains non-executable authority data unless the existing runtime owns
  the corresponding execution.

### Stage 8H - AIR-owned execution of qualified second-workload portions

Goal:

Execute meaningful known FLUX.2 workload portions through AIR's one runtime
without importing a second runtime authority.

Implementation order must follow cost and architectural value.

Candidate first AIR-owned portions:

- deterministic sigma schedule realization;
- seeded noise realization;
- latent-state initialization / transition bookkeeping;
- component resource preparation/residency transitions;
- any numerical component operation that can be implemented and qualified
  without importing an external workflow runtime.

Do not claim end-to-end FLUX.2 generation merely because AIR can describe its
plan.

If full text-encoder/denoiser/VAE numerical execution requires a larger
implementation than AIR 0.11 can responsibly qualify, Prompt 8 must state that
boundary precisely and still qualify the shared substrate work it actually
owns.

The release claim must match measured ownership.

### Stage 8I - second-workload evidence and falsification

Goal:

Compare AIR's new shared contracts against both qualified workloads.

Required questions:

1. Did any new type exist only because we imagined a future workload?
2. Did Qwen acquire image-specific vocabulary?
3. Did FLUX.2 acquire token/KV-specific vocabulary?
4. Is there still exactly one planner/capacity/graph/runtime/observation
   authority?
5. Can identified resources express both:
   - Qwen dense optional prepared state; and
   - text-encoder/denoiser/VAE component residency?
6. Does unknown transition duration remain unknown rather than zero?
7. Can AIR distinguish:
   - measured duration;
   - bounded chronology;
   - terminal residency state?
8. Does physical graph identity remain deterministic?
9. Can unsupported semantic meaning fail without guessing?

Any failure may require deleting or narrowing an abstraction before Prompt 8
closes.

## Initial type-direction hypothesis

This section is a hypothesis to guide Stage 8A review. It is not frozen API.

### Workload request boundary

Prefer an explicit discriminated representation over a bag of optional fields.

Conceptually:

```cpp
struct AutoregressiveRequestProfile {
    std::uint64_t prompt_tokens;
    std::uint64_t max_output_tokens;
    std::uint32_t active_sequences;
};

struct IterativeRequestProfile {
    std::uint32_t iteration_count;
    std::uint32_t active_instances;
};

using WorkloadRequestProfile =
    std::variant<AutoregressiveRequestProfile, IterativeRequestProfile>;
```

The existing `RequestProfile` may remain a compatibility alias during a
single-source migration if doing so does not create competing semantics.

Do not force both into a struct containing every possible field.

### Prepared resource state

Conceptually:

```cpp
struct PreparedResourceResidency {
    std::string resource_id;
    std::uint64_t device_bytes;
    bool resident;
};
```

This is intentionally smaller than a memory manager.

Potential future fields such as host bytes, device ID, lifetime class,
transition state, or provenance must be added only when the implementation
needs them.

The aggregate resident prepared byte count must be derived from these records.

### Work-unit identity

Conceptually:

```cpp
struct WorkAmount {
    WorkUnitKind kind;
    std::uint64_t count;
};
```

This prevents `work_units=4` from being silently interpreted as four tokens
when the FLUX.2 evidence means four denoising transitions.

## Relationship to existing Qwen policy types

The following remain valid Qwen physical policy:

- `SchedulingPolicy::prefill_quantum_tokens`;
- `KvPolicy`;
- `LinearPolicy`;
- `AttentionPolicy`;
- current legal Qwen operation sites and implementation families.

They should move under or remain referenced by the autoregressive physical
plan.

They should not be renamed into generic concepts unless FLUX.2 provides
evidence for that generic meaning.

For example:

- KV storage is not a universal "state store";
- prefill quantum is not a universal "phase quantum";
- quantized linear tactic is not a universal "component tactic".

## Relationship to the long-horizon physical optimizer

Prompt 8 is an important step toward AIR's longer-term role as a physical
computation compiler/runtime.

The eventual optimizer should be able to reason from:

```text
known semantic computation
+ identified resources
+ physical machine topology
+ dynamic environment
+ objective/constraints
+ qualified execution evidence
        ->
legal physical realization candidates
        ->
explicit execution / memory / residency plan
        ->
measured execution
        ->
new evidence
```

Prompt 8 should therefore make resource identity, residency, work amount, and
phase boundaries more explicit.

It should not yet attempt to solve every later physical optimization problem
such as:

- recompute versus transfer;
- CPU/GPU overlap;
- pinned staging;
- stream assignment;
- fusion;
- graph capture;
- tiling;
- prefetch;
- inter-device placement;
- energy/thermal objectives.

Those become legitimate search dimensions only after AIR owns enough physical
facts to evaluate them.

## Characterization requirements before refactor

Before modifying current public/internal contracts:

- characterize `RequestProfile` behavior;
- characterize manifest workload-region matching;
- characterize Strategy Lab cost/horizon behavior;
- characterize `RuntimeSnapshot` prepared-state behavior;
- characterize CapacityScheduler prepared-artifact handling;
- characterize ExecutionGraph serialization and identity;
- characterize observation serialization/meaning;
- characterize Qwen serving and generation APIs that construct these values.

The tests should preserve behavior, not implementation layout.

## Delete-first rule

When a new shared representation becomes authoritative:

- remove the superseded authored state;
- do not keep old and new independently mutable paths;
- provide a derived compatibility view only where required;
- migrate all internal producers before allowing new consumers.

## Prompt 8 qualification rules

Prompt 8 may close only if:

- all existing CPU tests pass;
- qualified CUDA/Qwen behavior remains nonregressed;
- Qwen numerical outputs remain unchanged where Prompt 8 does not explicitly
  alter numerical execution;
- FLUX.2-derived contracts are backed by retained Prompt 7 evidence;
- there is one resource/capacity authority;
- there is one execution graph authority;
- there is one runtime authority;
- no semantic meaning is guessed;
- evidence provenance is retained;
- unknown physical details remain unknown;
- at least one meaningful second-workload portion is AIR-owned and qualified;
- the exact release claim is narrower than or equal to what AIR actually
  executes.

## Stage 8A immediate implementation packet

Stage 8A is CURRENT.

It is deliberately smaller than the whole Prompt 8 program.

Implement:

1. characterization tests for current Qwen request/runtime/graph contracts;
2. explicit `WorkloadKind` and `WorkUnitKind`;
3. typed autoregressive and iterative request profiles;
4. a single discriminated workload request/profile boundary;
5. pure validation and inspection helpers;
6. zero numerical execution changes;
7. zero planner-policy changes;
8. zero scheduler-policy changes;
9. zero manifest-schema migration until the request boundary itself is
   characterized and tested.

Stage 8A exit:

- Qwen old behavior remains identical;
- iterative profile can be represented without token fields;
- invalid profile/workload combinations fail;
- no generic semantic catalog introduced;
- CPU preflight passes.

Only after 8A qualifies should Prompt 8B migrate prepared-resource residency.

## Prompt 8 exit

Prompt 8 closes when the second workload has forced and qualified genuine
shared AIR abstractions, not when AIR merely contains image-related type names.

The success condition is:

> Qwen2 and the qualified FLUX.2 workflow both lower through one AIR physical
> planning/resource/graph/runtime architecture, while each semantic adapter
> retains its own known meaning and AIR core does not guess unknown semantics.

Full end-to-end FLUX.2 numerical ownership is desirable but must not be claimed
unless implemented and qualified.


## Stage 8A implementation slice 1

Published source:

- `include/air/workload.hpp`;
- `src/runtime/workload.cpp`;
- `tests/workload_contract_tests.cpp`;
- CMake test target `air-workload-contract-tests`.

Design choices:

- existing `RequestProfile` is preserved as the qualified autoregressive
  contract;
- `AutoregressiveRequestProfile` is a compatibility alias to that exact type;
- `IterativeRequestProfile` contains only:
  - finite `iteration_count`;
  - `active_instances`;
- `WorkloadRequestProfile` is a discriminated `std::variant`;
- `WorkloadKind` currently contains only:
  - `autoregressive_tokens`;
  - `iterative_state`;
- `WorkUnitKind` currently contains only:
  - `tokens`;
  - `iterations`.

This slice deliberately does not add semantic value names, image dimensions,
tensor dtype, device placement, resource residency, manifest changes, planner
changes, scheduler changes, or backend changes.

The iterative validator rejects:

- zero iterations;
- zero active instances.

Existing autoregressive validity behavior is not retroactively tightened.

The dedicated characterization tests require:

- exact preservation of all existing `RequestProfile` fields through the new
  boundary;
- explicit token work-unit identity for Qwen;
- explicit iteration work-unit identity for the qualified iterative profile;
- stable inspection strings;
- failure for invalid iterative profiles.

Existing test suites remain the characterization authority for:

- Runtime/StaticPlanner request behavior;
- Strategy Lab request horizon and RuntimeSnapshot behavior;
- scheduler homogeneous request-shape behavior;
- ExecutionGraph serialization/identity;
- observation contracts.

Stage 8A remains open until the exact final source clears CPU preflight.
