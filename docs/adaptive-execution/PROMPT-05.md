# AIR 0.11 Strategy - Prompt 5

Status: CURRENT / CENSUS
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
