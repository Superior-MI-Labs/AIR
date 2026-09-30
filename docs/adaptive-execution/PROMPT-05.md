# AIR 0.11 Strategy - Prompt 5

Status: 5B CLOSED / QUALIFIED; 5C IMPLEMENTED / LIVE QUALIFICATION PENDING
Title: ExecutionGraph R0 - derived physical execution representation

## Qualified baseline

Prompts 1-4 are CLOSED / QUALIFIED.

Prompt 4 qualified source:

`d7087bafa0bacdc6d302cfe3eaf4ba18ec7853e8`

Prompt 4 established:

```text
qualified semantic operation site
        !=
physical implementation identity
```

and a typed legality authority over the existing backend capability storage.

## Prompt objective

Make AIR able to answer:

> What physical work does this qualified execution plan require on this
> prepared machine/backend, what are the dependencies between those steps, and
> what implementation/resource does each step use?

without creating a second production execution path.

ExecutionGraph R0 is derived physical execution data. It is not semantic model
truth.

## Two implementation approaches

### Approach A - derived graph over the existing execution path

```text
existing planner + ExecutionPlan
        +
PreparedModel capabilities
        +
qualified operation-site legality
        ->
immutable ExecutionGraph
```

The existing InferenceService, CapacityScheduler, MicrobatchScheduler,
PreparedModel, and SequenceState remain execution authorities.

Advantages:

- no second runtime;
- no scheduler duplication;
- no semantic behavior change;
- graph can be compared against real Prompt 3 timeline evidence;
- representation can be falsified before it controls execution;
- existing Qwen behavior remains the oracle for what the graph must describe.

Cost:

- the first graph may be deliberately coarse where current source does not yet
  expose a truthful physical distinction.

### Approach B - make runtime execution graph-driven immediately

Refactor the current serving/backend calls so the new graph becomes executable
as soon as it is introduced.

Rejected for the first Prompt 5 slice.

Reason:

- too much ownership movement at once;
- high risk of a second scheduler/executor during migration;
- graph bugs become runtime bugs before the representation is proven;
- easy to encode current Qwen accidents as universal graph semantics.

Decision:

**Use Approach A first.**

Execution may become progressively graph-driven only after a derived graph is
shown to faithfully represent current qualified execution and the migration can
replace, rather than duplicate, existing ownership.

## Current authority map

The census must preserve:

- `Planner` / existing adaptive planning: plan-selection authority;
- `ExecutionPlan`: current derived strategy data;
- `CapacityScheduler`: admission/resource-capacity authority;
- `MicrobatchScheduler`: worker-cycle ordering/budget authority;
- `PreparedModel`: prepared backend/model execution authority;
- `SequenceState`: per-sequence physical execution state;
- `InferenceService`: one production orchestration path;
- Prompt 3 execution spans: measured evidence, not plan truth;
- Prompt 4 operation-site queries: implementation-legality authority.

ExecutionGraph must own none of those responsibilities.

## 5A - exact execution-path census

Before adding a schema, map the current qualified Qwen physical path from:

```text
ExecutionPlan
  -> plan preparation
  -> sequence create/restore
  -> scheduler slice/batch
  -> prefill/decode backend call
  -> backend transfer/synchronization observations
  -> result/state commit
```

For each step record:

- current owner;
- inputs;
- resource/state touched;
- whether it is planned, runtime-decided, or merely observed;
- whether it is single-sequence or shared/batched;
- whether it can be represented deterministically before execution;
- whether its identity belongs in ExecutionGraph R0 or remains runtime state.

The census must explicitly inspect:

- plan preparation and prepared-artifact residency;
- sequence creation/restoration/checkpointing;
- single-sequence prefill;
- native multi-sequence prefill;
- single-sequence decode;
- native greedy decode batch;
- Prompt 4 linear/attention implementation choices;
- KV/state representation and page geometry;
- transfer enqueue boundaries;
- synchronization boundaries;
- cancellation/failure/transaction behavior.

## R0 representation rule

Do not invent a node merely because future AIR might need it.

Candidate physical concepts may include:

- compute;
- transfer;
- synchronization;
- allocation/residency/lifetime;
- preparation;
- explicit dependency;
- hardware placement;
- implementation identity.

Each candidate must map to a real current execution responsibility or be
deferred.

## Deterministic identity

ExecutionGraph R0 must have a deterministic identity for the same relevant
inputs.

The identity must not depend on:

- process addresses;
- wall-clock time;
- observation sequence numbers;
- request IDs when they do not affect physical structure;
- arbitrary container iteration order.

The census must determine which of these do affect graph identity:

- exact ExecutionPlan;
- prepared backend capability identity;
- topology fingerprint;
- relevant dynamic environment identity;
- request/workload shape;
- batch width;
- sequence-state representation.

Do not decide this from theory where current behavior can answer it.

## Planned versus observed truth

Prompt 5 must keep these distinct:

```text
ExecutionGraph
    = intended/derived physical execution structure

ExecutionSpan timeline
    = what was actually observed during execution
```

A future validator may compare the two.

Observed spans must never silently mutate the graph into retroactive plan truth.

## First falsification target

For the current Qwen path, derive a graph/projection and prove that it agrees
with the existing qualified execution contracts for:

- Reference single-sequence prefill/decode;
- CUDA single-sequence prefill/decode;
- CUDA native multi-sequence prefill where legal;
- CUDA native greedy decode batch where legal;
- exact selected Prompt 4 tactic legality;
- sequence-state/KV representation compatibility.

If a physical behavior cannot yet be represented truthfully, record the gap.
Do not fake precision.

## Forbidden in Prompt 5 census

Do not:

- add a universal semantic graph/IR;
- add a graph executor;
- add a second planner;
- add a second scheduler;
- add another hardware graph;
- move model truth out of ModelDefinition;
- move state ownership out of SequenceState/PreparedModel;
- optimize scheduling;
- add image/diffusion semantics;
- generalize KV into a speculative universal state abstraction;
- make browser JavaScript reconstruct graph truth.

## 5A stop condition

Stop the first slice when:

1. the current physical execution path and authority map are documented;
2. every proposed R0 graph field/node has a real current source owner;
3. two or more schema options have been compared;
4. graph identity inputs are characterized;
5. the smallest additive representation seam is identified;
6. no production execution behavior has changed;
7. the next implementation slice has explicit characterization tests.

Only then authorize the first ExecutionGraph type.


## 5A census result - graph boundary

The source census rejects a whole-request static graph derived only from
`ExecutionPlan`.

Reason:

Current physical execution shape is not fully determined at plan-selection
time. The existing production path makes additional legitimate runtime
decisions:

- `MicrobatchScheduler` selects worker-cycle slices from current active work;
- `InferenceService` groups compatible prefill slices into a native batch only
  when current participants permit it;
- compatible greedy decode sequences may become a native decode batch;
- exact-prefix restore depends on live reusable-state availability;
- output path differs between discard, full logits, device-greedy selection,
  and target-logprob scoring;
- cancellation/failure can terminate or prevent later physical work;
- plan preparation may be hot already or may materialize derived artifacts.

Therefore a graph predicted for the entire request before those decisions would
either encode speculation as fact or duplicate scheduler/runtime authority.

### Smallest truthful seam

The first canonical ExecutionGraph seam should be:

```text
Planner / ExecutionPlan
        ->
Capacity + Microbatch scheduling
        ->
existing compatibility grouping
        ->
PHYSICAL INVOCATION IS NOW CONCRETE
        ->
derive immutable ExecutionGraph
        ->
existing PreparedModel / SequenceState backend call
```

The graph is a pure description/projection of the physical invocation that is
about to occur.

It does not choose the batch, tactic, backend, output policy, or state hit.

Those decisions remain with their current owners.

## Schema alternatives

### Schema option 1 - whole-request expanded DAG

Materialize every expected physical step for an entire request at admission.

Advantages:

- visually simple notion of one request = one graph;
- potentially convenient for future whole-request optimization.

Rejected for R0.

Current execution is affected by later queue/batch/state decisions, so this
would require prediction, graph mutation, or scheduler duplication. It would
also expand autoregressive decode according to request/token behavior that is
not yet known.

### Schema option 2 - physical-invocation DAG

Create one immutable graph for one concrete physical invocation after current
scheduling/grouping decisions and before execution.

Examples:

- one single-sequence prefill invocation;
- one native multi-sequence prefill invocation;
- one single-sequence decode invocation;
- one native greedy decode batch;
- later, a plan-preparation invocation as a distinct physical scope.

Advantages:

- inputs are concrete rather than predicted;
- deterministic identity is practical;
- scheduler authority stays unchanged;
- graph can be directly compared with Prompt 3 observations;
- it gives Prompt 6 a physical object to transform experimentally;
- graphs can later be composed into larger schedules without making R0 lie.

Decision:

**Schema option 2 is the R0 direction.**

## Source-grounded physical distinctions

Current source supports these R0 distinctions without guessing:

### Backend and placement

`PreparedModel` owns backend execution. CUDA preparation is bound to a concrete
device ordinal. Hardware topology remains the canonical structural machine
description.

The graph may reference existing hardware-resource identities. It must not own
or rediscover topology.

### Operation implementation

Prompt 4 already owns legal operation-site implementation identity:

- prefill transformer-block linear;
- decode transformer-block linear;
- decode output projection;
- prefill attention;
- decode attention.

The graph references the already-selected legal implementation. It does not
select tactics.

### State representation

`SequenceState` and the prepared backend own physical sequence state.

CUDA currently uses paged KV state and checkpoint/fork semantics. The graph may
reference the active physical state representation and page geometry where they
affect execution compatibility, but it must not become the state owner.

### Transfers and synchronization

The CUDA backend currently exposes truthful host-observed physical boundaries
for:

- H2D prefill-token enqueue;
- H2D target-token enqueue;
- D2H logits enqueue;
- D2H greedy-result enqueue;
- D2H target-logprob/flag enqueue;
- explicit CUDA stream waits for logits, greedy selection, target logprobs, and
  outputless prefill.

These are valid candidates for graph transfer/synchronization nodes when the
selected output path requires them.

The current evidence does not justify claiming exact GPU kernel-duration nodes
from Prompt 3 observations.

### Preparation/residency

`PreparedModel::prepare_plan()`, tactic preparation-byte estimation, and
prepared-artifact accounting already own derived backend-global preparation.

Do not copy that metadata into a second graph registry.

Plan preparation should be represented later as a physical graph/invocation
derived from these authorities, not merged into per-sequence state.

## R0 identity inputs

For the first physical-invocation graph, candidate identity inputs are limited
to information that can change intended physical work:

- graph schema version;
- backend identity;
- selected `ExecutionPlan` physical fields relevant to the invocation;
- invocation kind: prefill/decode and single/batch;
- concrete participant/batch width;
- concrete token/work width where it changes physical work;
- output mode: discard/logits/greedy/target-logprobs;
- selected Prompt 4 implementation identities;
- physical state representation compatibility fields such as KV page geometry;
- hardware resource identity required for placement.

Explicitly excluded from structural graph identity unless later evidence proves
otherwise:

- request ID;
- sequence ID;
- wall-clock timestamps;
- observation sequence;
- planner reason strings;
- evidence/strategy labels that lower to identical physical work;
- process addresses.

Dynamic environment values such as free VRAM are planner/admission evidence.
They do not belong in graph identity merely because they were present when the
graph was derived.

## Smallest implementation seam

The first code should be additive and pure.

Recommended shape:

```text
existing concrete invocation data
        +
existing ExecutionPlan
        +
PreparedModel capabilities / backend identity
        +
existing hardware resource identity
        ->
derive_execution_graph(...)
        ->
immutable ExecutionGraph
```

The returned graph is inspected/tested/serialized only.

The existing backend call executes exactly as before.

No graph node dispatch exists in the first implementation slice.

## 5B characterization tests required before graph-driven execution

The first ExecutionGraph implementation should prove:

1. identical physical invocation inputs produce identical graph identity;
2. request/sequence IDs and observation timestamps do not change structural
   graph identity;
3. changing prefill implementation changes only the relevant prefill graph
   identity/content;
4. changing decode output implementation does not alter a prefill graph;
5. single versus native-batch invocation is explicit;
6. batch width/work width changes identity when physical work changes;
7. greedy versus full-logit versus target-logprob output paths produce the
   correct transfer/synchronization structure;
8. Reference graphs do not invent CUDA transfer/synchronization nodes;
9. invalid operation/tactic combinations are rejected through the existing
   Prompt 4 legality authority;
10. graph derivation does not execute a backend, allocate sequence state, mutate
    scheduler state, or change inference output;
11. Prompt 3 detailed observations can be compared to the graph without
    treating missing kernel-duration evidence as a failure.

## 5A status

5A census/architecture is complete enough to authorize an additive R0 schema
implementation.

5B is next.

5B must stop after an immutable graph type, pure derivation/projection, identity
rules, serialization/inspection, and characterization tests are green.

Do not make production execution graph-driven in 5B.


## 5B implementation result

ExecutionGraph R0 additive implementation is complete.

Implementation source head:

`257673c4e4206698fe0c6f1c979694b5761a7474`

Implemented boundary:

```text
already-concrete physical invocation
        +
existing ExecutionPlan
        +
existing BackendCapabilities
        +
existing topology-scoped placement
        ->
derive_execution_graph(...)
        ->
immutable ExecutionGraph R0
```

The implementation adds:

- schema-versioned `ExecutionGraph` derived data;
- explicit physical invocation kinds for single/native-batch prefill and
  single/native-greedy-batch decode;
- concrete participant work/output descriptors;
- topology fingerprint plus topology-local hardware resource placement;
- references to already-selected Prompt 4 linear/attention implementation
  identities;
- coarse compute, transfer, and synchronization regions;
- deterministic structural graph identity;
- stable inspection serialization;
- private graph construction so canonical identity is owned only by
  `derive_execution_graph()`.

The derivation function first delegates execution-plan legality to the existing
Prompt 4 authority.

It does not:

- schedule work;
- form a batch;
- choose a backend;
- choose an implementation;
- allocate or restore sequence state;
- prepare backend artifacts;
- execute a graph node;
- mutate runtime state;
- observe or rewrite execution evidence.

No production serving, scheduler, PreparedModel, CUDA, Reference, sequence-state,
or hardware-owner implementation was changed in 5B.

### Deliberate R0 precision limits

R0 remains coarse where the current source/evidence does not justify a stronger
claim.

In particular:

- model execution is represented as a physical compute region rather than an
  invented universal catalog of Qwen kernels;
- Prompt 3 host observations are not re-labeled as exact GPU kernel durations;
- output-transfer work quantities are retained only where the quantity is
  actually known;
- unknown full-logit payload size is not guessed by the graph;
- dynamic free memory and other environment measurements do not become
  structural graph identity.

### Topology-scoped identity

The census originally identified hardware resource identity as an R0 input.
Implementation review tightened this rule:

A resource ID such as `accelerator0` is topology-local and is not globally
meaningful by itself.

ExecutionGraph identity therefore binds:

```text
HardwareTopology fingerprint
        +
topology-local resource identity
```

while excluding volatile environment values such as current free VRAM.

## 5B characterization coverage

The core characterization tests now verify that:

1. identical concrete physical invocations produce identical graph identities
   and serialization;
2. already-consumed planner strategy labels and scheduling quantum do not
   fragment physical identity;
3. concrete work width changes graph identity;
4. topology identity scopes placement and changes graph identity;
5. a decode-output implementation change does not contaminate prefill graph
   identity;
6. changing the selected prefill implementation changes the prefill graph;
7. CUDA logits, device-greedy, target-logprob, and discard paths expose only
   the physical transfer/synchronization structure justified by current source;
8. native prefill and native decode batches remain structurally explicit;
9. Reference execution does not invent CUDA transfer/synchronization regions;
10. invalid tactic combinations are rejected through the existing Prompt 4
    legality authority;
11. missing topology/resource identity is rejected rather than guessed.

The tests emit:

`ExecutionGraph R0 characterization passed`

when the characterization block succeeds.

## 5B CPU preflight

GitHub Actions adaptive CPU preflight passed for the implementation and
qualification-script head:

`257673c4e4206698fe0c6f1c979694b5761a7474`

Workflow run:

`36660017292`

Results:

- adaptive shell syntax gate PASS;
- Release CPU build PASS;
- 13/13 CTests PASS;
- `ADAPTIVE_PREFLIGHT=PASS`.

## 5B live qualification authority

WolfCat CUDA qualification authority:

`scripts/qualify-adaptive-prompt5b.sh`

The qualifier requires:

- a clean source worktree;
- adaptive CPU preflight;
- a fresh CUDA Release build;
- the full CUDA CTest matrix;
- the explicit ExecutionGraph R0 characterization marker;
- existing real CUDA operation-site legality nonregression;
- existing renamed-source semantic/tactic independence nonregression;
- real-model CUDA generation;
- unchanged source worktree;
- retained machine/evidence identity and SHA-256 evidence checksums.

Run:

```text
bash scripts/qualify-adaptive-prompt5b.sh \
  ~/Models/AIR/Qwen2.5-1.5B-Instruct-Q4_K_M.gguf
```

Expected final gate:

`PROMPT5B_EXECUTION_GRAPH=PASS`

## 5B live qualification result

5B is CLOSED / QUALIFIED.

Qualified AIR handoff source:

`d154bf02225ba4b424c5b3734bfcb3d4845b6092`

WolfCat evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt5B-20260929-225827`

Terminal capture:

`/home/emerson/Downloads/AIR-0.11-Prompt5B-terminal.txt`

Qualification results:

- clean worktree before qualification;
- adaptive CPU preflight PASS;
- CPU 13/13 CTests PASS;
- fresh CUDA Release build PASS;
- CUDA 13/13 CTests PASS;
- `ExecutionGraph R0 characterization passed`;
- real prepared-CUDA operation-site legality PASS;
- Reference/CUDA semantic-binding parity PASS;
- renamed-source CUDA execution/tactic independence PASS;
- real Qwen2.5 CUDA generation PASS;
- selected backend remained CUDA;
- qualifier final gate `PROMPT5B_EXECUTION_GRAPH=PASS`;
- qualifier exit code 0;
- checksummed evidence retained by the qualifier.

5B therefore establishes a qualified immutable physical-invocation description
without moving execution authority.

## 5C current - read-only invocation integration and graph/evidence concordance

The next slice is deliberately narrower than graph-driven execution.

Objective:

1. derive the already-qualified ExecutionGraph at the exact point where an
   existing physical invocation is concrete;
2. expose/read that derived graph through one observational path;
3. correlate it with the existing Prompt 3 execution observations;
4. compare planned coarse regions against observed host-side transfer/sync
   evidence;
5. preserve the existing backend call as the only execution path.

5C must not:

- dispatch graph nodes;
- let the graph select batching, backend, tactics, or state;
- create a second scheduler or executor;
- mutate graph structure from observations;
- claim GPU kernel duration from host spans;
- create a parallel hardware/topology authority.

The first implementation step is source census of the exact single-prefill,
native-prefill-batch, single-decode, and native-decode-batch call seams plus
the existing observation correlation surface. No code movement is authorized
until those owners are re-verified against current source.


## 5C implementation result

5C read-only invocation integration is implemented.

Implementation and qualification-script source head:

`37feee09d6585245587a1ae055e1e47bdf618377`

### Exact integration seams

ExecutionGraph derivation now occurs only after the current production path has
already made the physical invocation concrete and immediately before the
existing backend call at:

- single-sequence prefill;
- native multi-sequence prefill;
- single-sequence generation decode;
- native greedy decode batch;
- decision target-logprob decode.

The graph does not form batches, select a backend, select tactics, restore
state, dispatch nodes, or change the existing backend call.

### Observation level

5C graph derivation is enabled only when:

`execution-observation=detailed`

Normal and off observation levels do not discover graph topology, derive
graphs, or retain graph observations.

This preserves the qualified AIR 0.11 normal-observation default until detailed
graph overhead has been measured.

### Topology ownership

Detailed mode obtains one structural topology snapshot from the existing
`discover_machine_topology()` authority.

Discovery occurs only after the existing backend/model preparation path has
completed, so graph observation cannot change backend preparation ordering.

Placement is resolved from that canonical topology:

- Reference uses the canonical CPU resource;
- CUDA uses the accelerator whose backend is `cuda` and whose ordinal equals
  the already-selected CUDA device.

ExecutionGraph does not rediscover or own topology.

### Planned versus observed truth

5C keeps two separate surfaces:

```text
GET /execution-graphs
    = intended derived physical invocation structure
      + correlation metadata
      + evidence-concordance result

GET /timeline
    = Prompt 3 execution spans actually observed
```

Observed spans never mutate the graph.

Graph evidence status is one of:

- `not-evaluated`: backend execution did not complete successfully;
- `concordant`: every coarse planned transfer/synchronization region has
  supporting observed evidence and no unsupported observed region appeared;
- `incomplete`: planned structure is not fully supported by retained
  correlated observation evidence;
- `contradictory`: observed transfer/synchronization evidence is incompatible
  with the planned coarse graph.

Graph derivation/evidence failure remains observational and non-fatal to
inference.

### Current negative evidence retained intentionally

Current CUDA shared native-batch input movement is not fully correlated through
Prompt 3's per-sequence observation sink.

Therefore the current characterization requires:

- single CUDA prefill/decode graph evidence to be `concordant`;
- native CUDA prefill/decode batch graph evidence to remain `incomplete`
  where shared token-transfer evidence cannot yet be associated with the
  invocation;
- no unsupported observed transfer/synchronization evidence.

This is a qualified evidence gap, not a reason to fabricate correlation.

### Bounded retention

Graph observations are bounded by the existing detailed execution-observation
capacity. Allocation/derivation failures increment explicit dropped/failure
counters instead of changing inference behavior.

### Characterization

CPU/reference tests prove:

- off mode derives no graphs;
- normal mode derives no graphs;
- detailed Reference graph observation uses canonical topology;
- detailed Reference graphs remain concordant without CUDA transfer/sync
  artifacts;
- correlation metadata remains outside structural graph identity;
- structured graph/evidence JSON is serialized independently of Prompt 3
  timeline JSON.

The real CUDA contract additionally exercises an atomic production cohort and
requires:

- current native prefill batching;
- current native greedy decode batching;
- concordant single-invocation graph evidence;
- preserved `incomplete` batch evidence for the known shared-correlation gap.

Expected CUDA contract marker:

`CUDA ExecutionGraph planned/observed concordance characterization passed`

### CPU preflight

GitHub Actions adaptive preflight passed at the complete 5C source/qualification
head.

Source:

`37feee09d6585245587a1ae055e1e47bdf618377`

Workflow run:

`36663922514`

Results:

- shell syntax gate PASS;
- Release CPU build PASS;
- 13/13 CTests PASS;
- `ADAPTIVE_PREFLIGHT=PASS`.

## 5C live qualification

Correctness/evidence authority:

`scripts/qualify-adaptive-prompt5c.sh`

This gate requires:

- CPU preflight;
- fresh CUDA Release build;
- CUDA 13/13 CTests;
- Prompt 5B graph characterization nonregression;
- real CUDA single/native-batch graph evidence characterization;
- real Qwen2.5 detailed-mode generation;
- structured `/execution-graphs` validation against canonical `/machine`;
- independent graph-node-to-`/timeline` transfer/synchronization checks;
- zero graph derivation failures;
- zero contradictory single-invocation evidence;
- normal-mode graph nonintrusion;
- clean worktree and checksummed evidence.

Expected final marker:

`PROMPT5C_GRAPH_EVIDENCE=PASS`

## 5C timing evidence

Timing must use the same method that qualified Prompt 3.

Authority:

`scripts/requalify-adaptive-prompt5c-overhead.sh`

This is intentionally a thin wrapper over
`scripts/requalify-adaptive-prompt3-overhead.sh`.

It therefore retains the same:

- 3x3 balanced Latin ordering;
- off/normal/detailed modes;
- warmups;
- six measured samples per session;
- three sessions per mode;
- median-of-session-medians analysis;
- GPU telemetry;
- clean-worktree and checksum evidence.

The 5C interpretation must compare current detailed-vs-normal overhead with the
qualified Prompt 3 evidence without claiming that any difference has a single
cause.

Expected final marker:

`PROMPT5C_OVERHEAD_REMEASURE=PASS`

## 5C first live attempt - qualification fixture falsified

The first WolfCat 5C qualification attempt failed before graph/evidence
validation.

Evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt5C-20260929-234809`

Source:

`b92dd505e9ca3dbcaac6ba04606483f8043e5589`

Observed result:

- adaptive CPU preflight PASS;
- CPU 13/13 CTests PASS;
- CUDA build PASS;
- CUDA 12/13 CTests PASS;
- `air-cuda-contract-tests` failed with:
  `CUDA graph cohort did not exercise native prefill/decode batching`;
- final gate:
  `PROMPT5C_GRAPH_EVIDENCE=FAIL`;
- failed stage:
  `cuda-ctest`;
- qualifier exit code 8;
- overhead remeasurement correctly did not run.

Root-cause analysis found a qualification-fixture error, not a production
runtime regression.

The fixture created an explicit CUDA service with default `ExecutionConfig`.
That means both decode linear tactics remained `baseline`.

Production native decode batching is intentionally eligible only when:

- the backend advertises native decode width greater than one;
- requests are deterministic greedy decode;
- and at least one selected decode linear implementation is non-baseline.

Therefore the fixture demanded native decode batching while configuring a plan
for which the current scheduler intentionally does not select that path.

The runtime eligibility rule is preserved.

Fix:

- configure the test cohort through the existing public `ExecutionConfig`
  contract with qualified `dense-f32-cublas` for decode block linear work;
- assert that the selected plan retained that tactic;
- report prefill and decode batch counters separately on failure.

Fix source:

`10fb4c65387fa6327ceb605d33746557aa87ae38`

This failure is retained because it demonstrates that 5C qualification must
prove the tested physical path was actually exercised rather than treating a
successful request as evidence for a path that never ran.

## 5C stop condition

Do not close 5C until both live evidence gates have been run and reviewed.

Do not repair the known native-batch observation gap inside 5C merely to make
the evidence green. If live evidence reproduces it, preserve it as the input to
the next architecture decision.

Do not begin graph-driven execution in the next slice.
