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
