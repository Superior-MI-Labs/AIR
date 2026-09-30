# AIR 0.11 Strategy - Prompt 4

Status: CLOSED / QUALIFIED
Title: Semantic operation / physical implementation boundary

## Prompt objective

Make AIR able to answer:

> For this qualified semantic operation site, which physical implementations
> are legal on this prepared backend?

without consulting source tensor names and without teaching every planner or
validator the internal layout of `BackendCapabilities`.

Prompt 4 does not create a universal operation IR.

## Qualified baseline

Prompts 1-3 are CLOSED / QUALIFIED.

Prompt 3 established a typed execution evidence surface with low measured
observation overhead under the balanced WolfCat experiment.

## Current coupling census

AIR currently has real operation scope, but it is encoded structurally as five
separate backend-capability fields:

```text
prefill transformer-block linear
decode transformer-block linear
decode output projection
prefill attention
decode attention
```

Physical implementation identities already exist:

```text
QuantizedLinearExecutionKind
AttentionExecutionKind
```

Current validation knows the exact storage layout:

```text
capabilities.prefill_block_quantized_linear
capabilities.decode_block_quantized_linear
capabilities.decode_output_quantized_linear
capabilities.prefill_attention
capabilities.decode_attention
```

That is the coupling Prompt 4 addresses first.

## 4A - qualified operation-site authority

Introduce one explicit operation-site identity containing only the five
currently qualified selectable sites.

Working shape:

```text
QualifiedOperationSite
  prefill_transformer_block_linear
  decode_transformer_block_linear
  decode_output_projection
  prefill_attention
  decode_attention
```

This is deliberately not called a universal neural operation enum.

Add query functions that return the physical implementation choices for one
site from a `BackendCapabilities` instance.

Requirements:

- operation identity is distinct from implementation identity;
- query does not inspect model tensor/source names;
- one site maps to exactly one implementation family;
- unsupported family/site queries fail explicitly;
- `validate_execution_plan()` delegates legality to this authority;
- existing 0.10/0.11 plan fields and tactic enums remain source-compatible.

## 4A implementation result

Slice 4A is implemented and CPU-preflight qualified.

Implementation:

- `QualifiedOperationSite` contains exactly five currently qualified
  Qwen execution sites:
  - prefill transformer-block linear;
  - decode transformer-block linear;
  - decode output projection;
  - prefill attention;
  - decode attention;
- `OperationImplementationFamily` distinguishes linear from attention
  implementation families;
- family classification rejects unknown operation-site values explicitly;
- `linear_implementations()` and `attention_implementations()` are the
  single typed legality queries over `BackendCapabilities`;
- implementation-family mismatches fail explicitly;
- `validate_execution_plan()` delegates tactic legality to the operation-site
  queries instead of directly knowing the five capability-vector fields;
- source tensor/model names do not participate in legality queries;
- public 0.10 tactic/plan fields remain source-compatible.

Contract coverage:

- exact site identity;
- site-to-family mapping;
- prefill/decode separation;
- output-projection isolation;
- implementation-family mismatch rejection;
- invalid-site rejection;
- existing plan-validation behavior.

Adaptive CPU preflight PASS:

`c77ba68eb85d9208c59197fab1d46ad3fabf6510`

A real prepared-CUDA capability contract was then added. The final pre-handoff
head, including qualifier hardening, passed adaptive CPU preflight at:

`d70bc2e891351aeae6899c6d8227ae066f2dc405`

## 4B decision - no new metadata registry

The census found that preparation/residency knowledge already has an owning
layer:

- `CudaPreparedModel` owns plan-preparation admission/accounting;
- `CudaExecutor` owns tactic preparation, trimming, and preparation-byte
  estimation;
- manifest evidence owns measured preparation/eviction cost.

Prompt 4 will not duplicate those facts into an operation metadata registry.

If the second architecture later proves shared implementation metadata is
required, it must be derived from those authorities rather than copied.

## 4B - implementation metadata

After 4A is qualified, determine whether the physical tactic identities need
small metadata for:

- backend;
- implementation family;
- preparation/residency requirements;
- batch-width constraints;
- evidence identity.

Do not add fields that are not used by current Qwen execution.

## 4C decision - defer standalone endpoint

A standalone `GET /operations` endpoint is intentionally deferred.

Reason:

Prompt 5 introduces the canonical physical `ExecutionGraph`. Creating an
operation-only transport surface immediately before that graph would likely
become duplicate/transitional GUI authority.

The future Control Room should consume operation legality as part of the
canonical execution representation rather than reconstructing or merging two
parallel views.

The core operation-site legality API remains available to Prompt 5 lowering and
validation.

## 4C - read-only operation capability surface

If 4A/4B establish a stable contract, expose a canonical read-only surface for
the future Control Room to answer:

- which semantic sites exist in the current qualified runtime;
- which implementations are legal on this backend;
- which implementation is selected by the active plan.

The browser must not reconstruct this mapping.

## 4D implementation

Live qualification authority:

`scripts/qualify-adaptive-prompt4.sh`

The qualifier performs:

1. mandatory CPU preflight;
2. fresh CUDA build;
3. full CUDA CTest matrix;
4. explicit real prepared-CUDA operation-site capability contract;
5. existing renamed-source CUDA semantic falsification;
6. short real-model CUDA generation;
7. active-plan tactic legality validation;
8. worktree cleanliness and bounded evidence checksums.

The qualifier deliberately asserts expected capability values as evidence, not
as runtime authority. Production legality remains owned by the typed
operation-site queries.

Pre-handoff audit caught and fixed:

- qualifier cleanup disabling shell `errexit` globally;
- checksum collection recursively hashing build trees;
- implicit invalid-site family fallback;
- missing explicit `<initializer_list>` dependency in the CUDA contract test.

These were repaired before live user handoff.

## 4D - qualification

Qualify:

- exact operation-site identity;
- implementation-family mismatch rejection;
- existing execution plans validate identically before/after refactor;
- Reference and CUDA capability sets remain truthful;
- source tensor names are absent from legality queries;
- strategy selection still cannot leak a block-only tactic into output
  projection;
- CPU-only and CUDA nonregression.

## Non-goals

Prompt 4 must not:

- define MatMul/Conv/FFT/etc. as a universal catalog;
- introduce a graph IR;
- rename every existing tactic type merely for aesthetics;
- encode Qwen tensor names into operation identity;
- change schedule policy;
- change kernel implementations;
- change model-family qualification.

## First implementation stop condition

Slice 4A stops when:

1. the five qualified operation sites exist;
2. capability queries are characterized by tests;
3. `validate_execution_plan()` uses the queries rather than direct field
   knowledge;
4. existing plan validation behavior remains unchanged;
5. adaptive CPU preflight passes.

Do not proceed into 4B until 4A is green.


## Final live qualification

Prompt 4 qualified on WolfCat-Studio from exact source:

`d7087bafa0bacdc6d302cfe3eaf4ba18ec7853e8`

Evidence directory:

`/home/emerson/Downloads/AIR-0.11-Prompt4-20260929-220128`

Results:

- adaptive CPU preflight: 13/13 CTests PASS;
- CUDA Release CTest matrix: 13/13 PASS;
- real prepared CUDA operation-site legality PASS;
- CUDA/Reference semantic-binding parity PASS;
- CUDA dense tactic remains independent of Qwen2 source tensor names;
- real Qwen2.5 CUDA generation PASS;
- selected backend: CUDA;
- selected prefill/decode linear and attention implementations were all within
  the qualified legal sets;
- qualifier exit code: 0;
- final gate: `PROMPT4_OPERATION_BOUNDARY=PASS`.

This closes Prompt 4.

The qualified result proves the narrow boundary AIR needs for Prompt 5:
physical execution representation can refer to qualified operation sites and
legal implementation identities without consulting source tensor names or
duplicating backend capability storage.

## Next prompt

Prompt 5 introduces ExecutionGraph R0 as derived physical execution data.

The first Prompt 5 slice is census/characterization only. It must not create a
second graph executor or move scheduling authority.
