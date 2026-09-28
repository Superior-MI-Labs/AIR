# AIR Modular Runtime R0 - Wave 4 Reference Migration

Status: IMPLEMENTED / AWAITING MACHINE QUALIFICATION

## Objective

Make the CPU Reference executor consume the prepared semantic model directly during numerical execution.

The reference path remains AIR's readable numerical oracle. This wave changes tensor addressing, not transformer mathematics.

## Before

```text
ReferenceExecutor
  -> construct Qwen2/GGUF source tensor names
  -> ModelDefinition::find_tensor(name)
  -> decode tensor
  -> execute Qwen2 reference math
```

## After

```text
ModelDefinition
  -> ArchitectureAdapter::prepare()
  -> PreparedModelSemantics
       |
       v
ReferenceExecutor
  -> semantic tensor descriptor
  -> decode tensor
  -> same reference math
```

## Implementation

`ReferenceExecutor` now retains one internal `PreparedModelSemantics` instance produced during construction.

The prepared view is derived from and lifetime-bound to the executor's canonical `ModelDefinition`.

The public header forward-declares the internal prepared type and stores it behind `std::unique_ptr`; the internal model contract is not promoted into AIR's public model API.

`ReferenceTensorReader` retains its existing public name-based diagnostic methods for compatibility.

Private descriptor-based operations are now available to `ReferenceExecutor`:

- rank-1 vector read;
- matrix row read;
- matrix-vector multiplication.

The executor uses only the descriptor-based path.

## Hot-path semantic bindings

Reference numerical execution now consumes:

- `token_embedding_weight`;
- per-layer attention normalization;
- per-layer query/key/value weights;
- optional query/key/value biases;
- per-layer attention output weight;
- per-layer FFN normalization;
- per-layer gate/up/down weights;
- final output normalization;
- explicit prepared output weight;
- optional output bias.

Tied output is no longer rediscovered with a source-name lookup during execution. It is represented explicitly by preparation.

## Source-level boundary check

At this wave's implementation head, `src/reference/reference_executor.cpp` contains:

```text
blk. tensor-name construction: 0
token_embd.weight literals:     0
output_norm.weight literals:    0
output.weight literals:         0
ModelDefinition::find_tensor:   0
```

The tensor reader still supports name lookup through its existing diagnostic/public methods. That compatibility surface is intentionally separate from ReferenceExecutor's production numerical path.

## Intentionally unchanged

- RMSNorm numerical formula;
- Q/K/V projection math;
- RoPE behavior;
- grouped-query causal attention;
- residual behavior;
- SwiGLU behavior;
- KV transaction and paging behavior;
- verification-stage ordering;
- sampling;
- tokenizer behavior;
- Reference generation behavior;
- CUDA numerical hot path;
- runtime scheduling/admission;
- SequenceState ownership;
- serving/Decision/HTTP/browser behavior;
- MEF and Builder contracts.

## Qualification authority

Wave 1 and Wave 3 characterization remain active, including:

- deterministic reference logits/tokens;
- verified-step parity;
- verification-stage count/order;
- KV state behavior;
- prefill/generation behavior;
- optional Q/K/V biases;
- optional output bias;
- tied output;
- architecture rejection;
- structural tensor validation;
- prepared semantic binding tests.

## Wave 4 exit gate

Wave 4 may close only when the exact branch HEAD:

1. builds in Release mode with CUDA enabled on WolfCat-Studio;
2. passes the complete CTest suite;
3. preserves the existing reference numerical characterization;
4. preserves verified/plain logit equality and verification-stage ordering;
5. keeps zero source tensor-name reconstruction and zero `find_tensor()` calls in `ReferenceExecutor`;
6. requires no scheduler, serving, CUDA, MEF, or Builder compensation.

If this gate passes, Wave 5 may migrate CUDA numerical tensor access to the same semantic contract.
