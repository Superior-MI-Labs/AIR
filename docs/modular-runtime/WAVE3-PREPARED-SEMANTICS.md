# AIR Modular Runtime R0 - Wave 3 Prepared Semantic Model

Status: CLOSED / QUALIFIED

## Objective

Resolve model-family source tensor names and validated geometry into one immutable semantic preparation product before backend execution.

This wave does not introduce a neural graph IR and does not change Reference or CUDA numerical execution.

## Contract

The internal prepared model contains:

- canonical source-model identity;
- architecture identity;
- validated model geometry;
- derived head dimension;
- token-embedding semantic binding;
- output-normalization semantic binding;
- explicit output-weight binding, including tied-output identity;
- optional output bias;
- one typed binding set per transformer layer;
- optional Q/K/V biases;
- source TensorDescriptor identity for provenance.

Conceptually:

```text
source tensor name
blk.0.attn_q.weight
        |
        v
Qwen2 architecture adapter
        |
        v
semantic role
layer[0].query_weight
        |
        +--> original TensorDescriptor retained for provenance
```

## Authority

`ModelDefinition` remains canonical model truth.

`PreparedModelSemantics` is derived state. It does not own or mutate model tensors.

The source `ModelDefinition` must outlive the internal prepared semantic view.

## Preparation path

```text
ModelDefinition
      |
      v
resolve_model_architecture()
      |
      v
Qwen2ArchitectureAdapter::prepare()
      |
      +-- canonical ModelDefinition validation
      +-- Qwen2 semantic/geometry validation
      +-- source-name resolution
      +-- typed semantic tensor bindings
      |
      v
PreparedModelSemantics
```

Architecture adapters now have one model operation: `prepare()`.

The transitional Wave 2 split between validation and execution-tensor enumeration has been removed.

## Current executor use

Reference and CUDA initialization now use `PreparedModelSemantics::execution_tensors()` for supported-encoding checks and CUDA residency preparation.

Their numerical hot paths still reconstruct current Qwen2 source names.

That remaining coupling is intentional:

- Wave 4 migrates Reference numerical execution to semantic bindings.
- Wave 5 migrates CUDA numerical execution to semantic bindings.

## Characterization

Wave 3 adds direct tests for:

- Qwen2 architecture resolution;
- prepared source-model identity;
- validated geometry and derived head dimension;
- explicit tied-output binding when `output.weight` is absent;
- absent optional output bias;
- semantic Q/K/V role binding;
- preservation of source TensorDescriptor names for provenance;
- absent optional Q/K/V bias bindings;
- present optional Q/K/V tensors becoming explicit semantic bias roles;
- canonical duplicate tensor identity being rejected before semantic preparation;
- de-duplicated execution tensor enumeration for tied output.

Existing Wave 1 tests continue to protect:

- missing required tensor rejection;
- wrong tensor-shape rejection;
- optional Q/K/V bias execution;
- optional output bias;
- wrong architecture;
- unsupported RoPE scaling;
- unsupported sliding-window attention.

## Intentionally unchanged

- Qwen2 is still the only production architecture.
- GGUF is still the current model-container source.
- Reference transformer math is unchanged.
- CUDA kernels and transformer math are unchanged.
- Scheduler/admission/SequenceState are unchanged.
- Serving and chat rendering are unchanged.
- Decision is unchanged.
- Public AIR API is unchanged.
- MEF R0 is unchanged.
- Builder is unchanged.

## Wave 3 qualification result

WolfCat-Studio qualification reported:

```text
Release build
CUDA=ON
12/12 CTests PASS
0 failures
```

Prepared semantic bindings therefore qualified without introducing a second model-truth path, public architecture claim, or runtime authority.

Wave 3 is CLOSED / QUALIFIED.

Wave 4 is authorized to migrate Reference numerical execution to the stored semantic bindings.
