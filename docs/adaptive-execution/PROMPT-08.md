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
enum class ExecutionWorkloadKind {
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

using ExecutionWorkloadProfile =
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

Stage 8A is CLOSED / QUALIFIED.

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
- `ExecutionWorkloadProfile` is a discriminated `std::variant`;
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

Stage 8A qualified at source
`0768b749d466f8084add09088be993060f63a564`.

Adaptive CPU preflight run `36930036554` completed SUCCESS.

Stage 8A is CLOSED / QUALIFIED.


## Stage 8A preflight failure and ownership correction

Exact failing source:

`31c4f62b06ef8c48e56c4f8a24be2101829ed164`

GitHub Actions run:

`36929261081`, attempt 2.

Classification:

CONTRACT OWNERSHIP / SYMBOL COLLISION.

The new Prompt 8A header originally introduced `air::WorkloadKind`.
AIR already owns that name in `air/decision.hpp` for a different semantic
axis:

- `generation`;
- `bounded_decision`.

That existing type classifies service/request semantics.

Prompt 8A needs a separate execution-structure axis:

- autoregressive token recurrence;
- iterative state transformation.

Those axes must not be collapsed. A bounded Decision request currently lowers
through the same autoregressive token machinery as Generation, so service
semantic kind is not execution structure.

The linker correctly rejected duplicate
`to_string(air::WorkloadKind)` definitions.

Correction:

- retain existing `air::WorkloadKind` unchanged;
- rename the new physical-structure enum to
  `air::ExecutionWorkloadKind`;
- rename the discriminated planning profile to
  `air::ExecutionWorkloadProfile`;
- tests now include `air/decision.hpp` and assert the two enum types remain
  distinct;
- no planner, scheduler, manifest, backend, or numerical behavior changes are
  introduced by the correction.

A mechanical intermediate rename briefly produced
`ExecutionExecutionWorkloadKind` in the header. This was caught during source
review before the next qualification run and corrected. It is retained here as
process evidence for why exact-head preflight remains mandatory.


## Stage 8B slice 1 - prepared-resource identity contract

Status: CURRENT

Prompt 6 could safely treat prepared state as anonymous bytes only because the
qualified strategy effectively had one optional prepared artifact.

Prompt 7 falsifies that assumption.

Text encoder, denoiser, and VAE are independently meaningful resources. Equal
resident byte totals from different resources cannot establish that a required
component is hot.

The same issue already exists below Qwen's aggregate interface: the CUDA
executor owns distinct dense-FP32 and q5q8-DP4A prepared arenas even though
`PreparedModel::prepared_artifact_device_bytes()` currently exposes only their
sum.

Slice 1 therefore introduces identity before changing runtime ownership:

- `PreparedResourceRequirement`;
- `PreparedResourceResidency`;
- explicit `unknown | nonresident | resident` residency state;
- optional expected device bytes on requirements;
- measured resident device bytes on residency records;
- validation requiring unique non-empty IDs;
- identity-aware requirement satisfaction;
- aggregate resident bytes as a derived calculation;
- deterministic projection of current CUDA prepared linear tactics to stable
  resource IDs.

Important rule:

`same bytes != same resource`

Generic AIR resource logic treats resource IDs as opaque equality keys. It does
not parse names to recover semantic meaning.

Known current physical IDs:

- `cuda/linear/dense-f32-cublas`;
- `cuda/linear/q5q8-dp4a-hybrid`.

These identify current optional CUDA prepared artifacts. They are not universal
semantic operation names.

This first slice intentionally does not yet migrate `RuntimeSnapshot`,
Strategy Lab manifests, backend residency reporting, or capacity admission.
Those remain the next Stage 8B slice after this resource identity contract is
qualified.

The legacy aggregate prepared-artifact path therefore remains temporarily
authoritative in runtime planning during slice 1. No new producer writes an
identified residency set yet, so there is not yet a second mutable runtime
truth.

Stage 8B slice 1 exit:

- wrong-resource/equal-byte substitution is rejected;
- unknown/nonresident state cannot be interpreted as hot;
- multiple component residencies can coexist;
- aggregate bytes are derivable from identified resident records;
- current Qwen prepared tactic requirements deduplicate by stable identity;
- no Qwen numerical/planner/scheduler behavior changes;
- Adaptive CPU preflight passes.


## Stage 8B slice 1 qualification

Qualified source:

`5b544b0a7c39ef0923abbe9a95b59db180d6f5e1`

Adaptive CPU preflight:

`36930733784` -> SUCCESS.

The pure resource identity contract is qualified.

## Stage 8B slice 2 - backend-owned identified residency

Status: CURRENT

Slice 2 connects the qualified resource contract to the existing prepared
backend authority.

`PreparedModel` now exposes an additive
`prepared_resources()` observation surface.

Rules:

- the backend remains the resource-state owner;
- resource records are projections of existing backend state;
- the compatibility aggregate is not independently stored;
- reference backend owns no optional prepared resources;
- CUDA reports current legal/supported optional prepared resources by stable
  identity;
- current dense-FP32 bytes are derived from the executor's existing aggregate
  prepared-linear counter minus the existing DP4A counter;
- DP4A residency uses its existing dedicated counter;
- unsupported and absent resources are not fabricated as required plan state.

The live CUDA contract now requires this transition:

```text
dense resource: nonresident
        ->
prepare dense plan
        ->
same resource identity: resident with non-zero device bytes
        ->
trim to baseline plan
        ->
same resource identity: nonresident with zero resident bytes
```

The identified-residency byte sum must match the legacy aggregate compatibility
view while that view still exists.

Slice 2 does not change Strategy Lab hot-state policy yet. That migration is
intentionally deferred until live CUDA proves the backend observation surface
is truthful.

Qualification order:

1. Adaptive CPU preflight;
2. fresh WolfCat CUDA build;
3. full CUDA CTest;
4. explicit `air-cuda-contract-tests` identified-resource transition marker.

Only then may Stage 8B migrate `RuntimeSnapshot` and planner hot-state logic.


## Stage 8B slice 2 CPU preflight and WolfCat handoff

Slice-2 implementation source:

`604ebbd6bb35e2df1ef6b9845ba6d2b294cf2926`

Adaptive CPU preflight:

`36931183589` -> SUCCESS.

The remaining qualification is physical CUDA evidence.

Canonical qualifier:

`scripts/qualify-adaptive-prompt8b-resources.sh`

The qualifier requires:

- clean AIR worktree;
- active adaptive-execution branch;
- clean GPU compute baseline;
- adaptive CPU preflight;
- fresh Release CUDA build;
- full CUDA CTest;
- explicit prepared-resource residency transition marker;
- existing CUDA operation-site and renamed-source nonregression markers;
- real Qwen CUDA generation;
- no remaining GPU compute process;
- clean source worktree;
- retained evidence checksums.

Expected final marker:

`PROMPT8B_PREPARED_RESOURCE_RESIDENCY=PASS`

WolfCat qualified Stage 8B slice 2.

Evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt8B-Resources-20261001-184458`

Final marker:

`PROMPT8B_PREPARED_RESOURCE_RESIDENCY=PASS`

Qualified facts:

- adaptive CPU preflight: 15/15 PASS;
- fresh CUDA CTest: 15/15 PASS;
- CUDA prepared backend operation-site legality PASS;
- identified prepared-resource residency transition PASS;
- CUDA/reference semantic-binding parity PASS;
- ExecutionGraph planned/observed concordance PASS;
- renamed-source dense tactic independence PASS;
- real Qwen CUDA nonregression PASS;
- selected backend CUDA;
- generated tokens: 4;
- evidence checksums retained.

Stage 8B slice 2 is CLOSED / QUALIFIED.


## Stage 8B slice 3 - planner residency authority migration

Status: CURRENT

Slice 3 removes anonymous prepared bytes as authored runtime planner state.

The already-qualified `PreparedResourceResidency` record is extracted into
`resource_state.hpp` so both runtime snapshots and resource helpers use the
same type without a circular dependency.

`RuntimeSnapshot` now owns:

- free device memory;
- resident KV bytes;
- identified prepared-resource residency records;
- current strategy identity;
- device utilization.

It no longer owns an authored `prepared_artifact_bytes` field.

Strategy Lab changes:

- runtime resource records are validated before planning;
- invalid residency state causes conservative fallback;
- candidate hot-state requires exact prepared-resource identity;
- equal bytes from the wrong resource cannot establish hot state;
- current Qwen single-resource manifest bytes become expected-byte evidence
  only when the plan has exactly one prepared resource;
- unknown multi-resource byte splits are not invented;
- eviction evidence is required only when a candidate would actually drop a
  resident prepared resource required by the incumbent plan;
- transition-cost estimation uses the same identity-aware facts.

Capacity limitation retained deliberately:

Manifest `prepared_artifact_bytes` remains aggregate measured capacity
evidence for this slice.

If a future candidate requires several resources and only a subset is already
resident, AIR conservatively treats the candidate as cold and charges the full
aggregate preparation bytes. It does not guess a per-resource byte split.

Serving changes:

- planner snapshots are populated from backend `prepared_resources()`;
- service snapshots expose current identified resources;
- `current_prepared_artifact_bytes` remains a derived diagnostic total;
- `/runtime` additionally exposes `current_prepared_resources`.

The backend compatibility method
`prepared_artifact_device_bytes()` remains derived from the same executor
counters but is no longer the planner residency authority.

Slice 3 falsification requirements:

1. hot dense resource remains hot;
2. same-byte wrong resource remains cold;
3. invalid resource state causes conservative fallback;
4. cold/hot break-even behavior otherwise remains characterized;
5. missing eviction evidence is required only for an actually resident
   incumbent resource that would be dropped;
6. CPU preflight passes;
7. fresh CUDA Qwen qualification later confirms planner/runtime integration.


## Stage 8B slice 3 CPU qualification and CUDA replay handoff

Implementation source:

`97078266d410d4a33a36be1ecac8fd2b1cbdb66e`

Adaptive CPU preflight:

`36939282676` -> SUCCESS.

The next gate reuses retained qualified Prompt 6C strategy evidence rather than
rerunning its full benchmark census.

Retained strategy oracle:

`/home/emerson/Downloads/AIR-0.11-Prompt6C-Transitions-20260930-100245`

Canonical focused qualifier:

`scripts/qualify-adaptive-prompt8b-planner-resources.sh`

It performs:

- clean GPU baseline;
- current CPU preflight;
- fresh CUDA build and full CTest;
- retained-manifest hot-dense replay;
- retained-manifest dense -> reuse8 eviction replay;
- exact resource-state validation from Strategy Probe snapshots;
- bounded evidence checksums.

Required hot-dense evidence:

- dense selected twice;
- second request reports prepared-state hot;
- second request forecasts zero preparation bytes;
- exact resource ID
  `cuda/linear/dense-f32-cublas`
  is resident before and after the second request;
- derived aggregate bytes equal that resource's measured resident bytes.

Required eviction evidence:

- dense selected first;
- reuse8 selected second;
- dense resource is resident before the second request;
- positive eviction duration is measured;
- the same dense resource is nonresident with zero resident bytes after;
- derived aggregate prepared bytes are zero;
- no hidden dense residency remains.

Expected final marker:

`PROMPT8B_PLANNER_RESOURCE_AUTHORITY=PASS`

This is a focused replay of the planner behavior affected by slice 3. It is not
a new performance qualification and does not replace Prompt 6 evidence.


## Stage 8B slice 3 qualification

WolfCat evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt8B-Planner-Resources-20261001-193414`

Final marker:

`PROMPT8B_PLANNER_RESOURCE_AUTHORITY=PASS`

Qualified results:

- full CUDA CTest: 15/15 PASS;
- hot dense identity replay PASS;
- dense -> reuse8 eviction identity replay PASS;
- exact dense resource ID:
  `cuda/linear/dense-f32-cublas`;
- measured hot dense residency: `5240782848` bytes;
- measured eviction: `2.401996 ms`;
- hidden dense residency after eviction: NO;
- evidence checksums retained.

Stage 8B is CLOSED / QUALIFIED.

## Stage 8C slice 1 - explicit work-unit identity

Status: CURRENT

The initial Stage 8A hypothesis contained only:

- tokens;
- iterations.

Review of the existing qualified CUDA observer falsified that as incomplete
before implementation.

Existing CUDA transfer observations already store byte counts in the field
named `work_units`, including:

- input token buffer bytes;
- logits buffer bytes;
- greedy-result bytes;
- target-token buffer bytes;
- target-logprob result bytes;
- target-logprob flag bytes.

Therefore Prompt 8C adds only one newly proven unit:

- `bytes`.

No FLOP, tensor-element, pixel, sample, frame, or other imagined unit is added.

Slice-1 contract:

- `WorkUnitKind` moves to an independent work-unit contract;
- supported proven kinds are `tokens | iterations | bytes`;
- every non-zero work measure requires a unit;
- zero-work administrative/synchronization spans may omit a unit;
- `PhysicalInvocation` carries an explicit work-unit kind;
- current prefill/decode invocation vocabulary accepts only token work;
- iterative work is not forced through token-specific invocation kinds;
- execution observation spans carry an optional unit alongside the count;
- Qwen service compute/request work is reported as tokens;
- CUDA transfer work is reported as bytes;
- zero-work synchronization spans remain unitless;
- execution graph and observation schemas advance because typed and untyped
  counts are different contracts;
- graph canonical identity includes invocation work-unit kind;
- node-local `ExecutionGraphNode.work_units` remains unchanged in this slice.

The node-local field is deliberately deferred. Existing graph nodes use
heterogeneous local counts such as target count and greedy participant count.
Relabeling those as tokens would be false. Stage 8G ExecutionGraph R1 owns that
cleanup after Stage 8D establishes workload-scoped invocation vocabulary.

Slice-1 falsification:

1. missing invocation work-unit kind fails graph derivation;
2. iterative units on current token-specific prefill/decode vocabulary fail;
3. deterministic Qwen graph identity includes `tokens`;
4. non-zero untyped observations are invalid;
5. current CUDA transfer observations report `bytes`;
6. current Qwen service observations report `tokens`;
7. CPU preflight passes before live CUDA qualification.


## Stage 8C slice 1 first preflight result

Source:

`bb274cd7666452ab63d87b6c34990cafd81a616e`

Adaptive CPU preflight:

`36942384084` -> FAIL at CTest.

Classification:

CHARACTERIZATION FIXTURE MIGRATION.

Build/link succeeded. 13/15 tests passed.

Failures:

- `air-core-tests`:
  - native prefill batch fixture omitted work-unit kind;
  - native decode batch fixture omitted work-unit kind;
- `air-protocol-tests`:
  - protocol graph fixture omitted work-unit kind.

This is the intended new validation firing on legacy direct constructors. No
production runtime path was shown to be untyped.

Correction:

- remaining Qwen graph fixtures explicitly declare `tokens`;
- protocol observation fixture explicitly declares token work;
- protocol JSON tests now require `work_unit_kind: "tokens"`;
- the protocol graph JSON test requires token work-unit identity;
- `validate_work_measure` is not `noexcept`, because constructing an error
  Status may allocate.

The negative characterization that intentionally omits a work-unit kind remains
and must continue to fail graph derivation.


## Stage 8C slice 1 corrected CPU qualification

Corrected exact source:

`6984b768088a687548c2cdcf514ba38eee2d0260`

Adaptive CPU preflight:

`36942691588` -> SUCCESS.

The fixture-migration failure is closed without weakening the explicit-unit
contract.

Canonical WolfCat qualifier:

`scripts/qualify-adaptive-prompt8c-work-units.sh`

Live qualification requires:

- clean GPU compute baseline;
- exact current CPU preflight;
- fresh CUDA build;
- full CUDA CTest;
- ExecutionGraph characterization;
- CUDA graph/observation concordance contract;
- real Qwen generation under detailed observation;
- timeline schema v2;
- every non-zero service work count typed `tokens`;
- every non-zero CUDA transfer work count typed `bytes`;
- zero-work CUDA synchronization spans unitless;
- every current Qwen ExecutionGraph invocation typed `tokens`;
- graph schema v2;
- no graph derivation failures;
- graph evidence concordant;
- clean GPU/process/worktree teardown;
- bounded evidence checksums.

Expected final marker:

`PROMPT8C_TYPED_WORK_UNITS=PASS`

Stage 8C slice 1 remains OPEN until that live evidence passes.


## Stage 8C slice 1 WolfCat qualification

Evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt8C-WorkUnits-20261001-202558`

Final marker:

`PROMPT8C_TYPED_WORK_UNITS=PASS`

Qualified facts:

- Adaptive CPU preflight: 15/15 PASS;
- fresh CUDA CTest: 15/15 PASS;
- ExecutionGraph R0 characterization PASS;
- CUDA operation-site, prepared-resource, parity, graph concordance, and
  renamed-source contracts PASS;
- real detailed Qwen observation:
  - non-zero service spans: 5, all `tokens`;
  - non-zero backend transfer spans: 5, all `bytes`;
  - backend synchronization spans: 4, all zero-work/unitless;
  - graph observations: 4, all current Qwen invocation work `tokens`;
  - graph concordance PASS;
- topology fingerprint:
  `hardware-topology:v1:f521e9bdd95ed645`;
- physical hardware resource:
  `gpu0`;
- evidence checksums retained.

Stage 8C slice 1 is CLOSED / QUALIFIED.

## Stage 8D slice 1 - discriminated workload-scoped physical invocation

Status: CURRENT

The first design decision is to avoid extending the existing flat Qwen
`PhysicalInvocationKind` and token-only `PhysicalOutputMode` enums with
iterative/image values.

That flat-enum approach would permit structurally illegal combinations such as
an iterative denoising invocation with logits/greedy output vocabulary.

Instead Stage 8D chooses the discriminated direction already allowed by the
Prompt 8 plan:

`AutoregressivePhysicalInvocation | IterativePhysicalInvocation`

Slice-1 rules:

- existing `PhysicalInvocation` remains the already-qualified Qwen member and
  is named by the additive alias `AutoregressivePhysicalInvocation`;
- `IterativePhysicalInvocation` carries only:
  - finite iteration count;
  - active instance count;
  - topology fingerprint;
  - hardware resource identity;
- the iterative type has no token participant list;
- the iterative type has no logits/greedy/target-logprob output field;
- its work-unit interpretation is fixed structurally to `iterations`;
- no conditioning/noise/sigma/latent/image semantic values enter this type;
- no text-encoder/denoiser/VAE component identity enters this type yet;
- no second graph, executor, planner, scheduler, or resource authority is
  introduced;
- ExecutionGraph R0 remains autoregressive-only in this slice.

The shared boundary validates only discrimination-level invariants. It does not
duplicate detailed Qwen prefill/decode legality already owned by
`derive_execution_graph`.

Stage 8D slice-1 falsification:

1. Qwen member remains exactly the existing physical invocation type;
2. iterative member cannot express token participant/output vocabulary;
3. qualified four-iteration shape validates;
4. zero iterations/instances fail;
5. missing topology/resource placement fails;
6. untyped autoregressive work fails the Stage 8C invariant;
7. shared validation does not become a second Qwen graph validator;
8. CPU preflight passes with all existing tests plus the new invocation
   contract test.
