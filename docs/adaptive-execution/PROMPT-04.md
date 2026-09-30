# AIR 0.11 Strategy - Prompt 4

Status: IN PROGRESS
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

## 4B - implementation metadata

After 4A is qualified, determine whether the physical tactic identities need
small metadata for:

- backend;
- implementation family;
- preparation/residency requirements;
- batch-width constraints;
- evidence identity.

Do not add fields that are not used by current Qwen execution.

## 4C - read-only operation capability surface

If 4A/4B establish a stable contract, expose a canonical read-only surface for
the future Control Room to answer:

- which semantic sites exist in the current qualified runtime;
- which implementations are legal on this backend;
- which implementation is selected by the active plan.

The browser must not reconstruct this mapping.

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
