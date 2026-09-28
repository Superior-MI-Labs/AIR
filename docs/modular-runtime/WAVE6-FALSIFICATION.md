# AIR Modular Runtime R0 - Wave 6 Abstraction Falsification

Status: IMPLEMENTED / REQUALIFICATION REQUIRED

## Objective

Attempt to falsify the claim that AIR execution now depends on semantic tensor roles rather than Qwen2/GGUF source tensor names.

This wave does not add another production model family and does not broaden public Qwen2 source compatibility.

## Falsification strategy

Construct test-only Qwen2 computation fixtures whose tensor bytes and shapes preserve the qualified computation while the source tensor names are deliberately replaced with unrelated identifiers.

Examples include:

```text
canonical source                    alias source
----------------                    ------------
token_embd.weight                   alien.embedding
output_norm.weight                  alien.final_norm
blk.0.attn_q.weight                 projection.query
blk.0.attn_k.weight                 projection.key
blk.0.attn_v.weight                 projection.value
blk.0.ffn_gate.weight               mlp.gate
```

The public architecture adapter is intentionally not taught these aliases.

Therefore:

- normal public `ReferenceExecutor::create(alias_model)` must fail;
- an explicitly prepared semantic view over the same alias model may execute through the internal prepared-executor seam.

That distinction proves tensor-name independence below preparation without falsely claiming expanded public format/model support.

## Single construction paths

Wave 6 introduces internal executor factories:

```text
ReferenceExecutorFactory
CudaExecutorFactory
```

They are not installed public model APIs.

The normal public constructors still:

```text
ModelDefinition
  -> architecture adapter
  -> prepare semantic model
  -> internal executor factory
  -> existing executor
```

The falsification tests enter only at the already-prepared step.

There is no second numerical executor, scheduler, model registry, or model arena.

## Prepared semantic integrity

A new internal invariant validator checks that a prepared semantic view:

- references a canonical `ModelDefinition`;
- uses only the currently qualified `qwen2` architecture;
- preserves canonical model geometry exactly;
- preserves derived head geometry;
- preserves current qualified Qwen2 RoPE/RMS/sliding-window restrictions;
- has the expected layer count;
- binds every required semantic role;
- binds only descriptors owned by the canonical model;
- uses the expected tensor shapes for each role;
- models tied output explicitly;
- contains no duplicate physical tensor descriptor across execution roles, except the separately represented tied-output relationship.

This validator is used by both internal executor factories.

The prepared seam therefore cannot bypass the qualified Qwen2 semantics merely because it bypasses source-name resolution.

## Reference falsification

The reference test suite now verifies that:

1. an alias model contains no canonical Qwen2 execution tensor names;
2. public Qwen2 preparation rejects that renamed model;
3. manually prepared semantic bindings over the renamed model validate;
4. the same Reference executor construction path accepts those prepared bindings;
5. canonical and renamed-source execution produce equivalent logits.

It also attacks:

- missing required semantic roles;
- incorrect derived head geometry;
- geometry diverging from canonical `ModelDefinition`;
- duplicate physical descriptors across semantic roles;
- forged tied-output binding;
- descriptors taken from another model;
- unsupported RoPE scaling;
- unsupported sliding-window attention;
- an unqualified architecture identifier.

## CUDA falsification

On a CUDA-capable machine, the existing CUDA contract test additionally:

1. constructs a renamed-source semantic fixture;
2. validates the prepared semantic model;
3. creates CUDA execution through the internal prepared factory;
4. prepares the dense-FP32 linear tactic using semantic roles;
5. executes renamed-source CUDA decode;
6. compares the logits to the canonical Reference oracle.

This specifically attacks the former hidden source-name dependency in prepared-linear tactic selection as well as the main CUDA numerical path.

## Source boundary

At the Wave 6 implementation head:

`src/reference/reference_executor.cpp`

contains no hard-coded canonical Qwen2 tensor source names and no `find_tensor()` call.

`src/cuda/cuda_backend.cu`

contains no hard-coded canonical Qwen2 tensor source names and no `find_tensor()` call.

Qwen2 source-name knowledge remains where it belongs:

```text
src/model/qwen2_contract.cpp
```

The Reference executor still contains Qwen2-specific error wording for the currently qualified RoPE semantics. That is computation semantics, not source tensor-name binding.

## Intentionally unchanged

- Qwen2 remains the only qualified production architecture.
- Public Qwen2 source tensor naming remains unchanged.
- GGUF loading is unchanged.
- Reference numerical math is unchanged.
- CUDA kernels and device-memory ownership are unchanged.
- scheduler/admission/SequenceState are unchanged.
- serving/Decision/HTTP/browser are unchanged.
- public AIR model support claims are unchanged.
- MEF R0 is unchanged.
- Builder is unchanged.

## First machine qualification result

WolfCat-Studio reached the exact Wave 6 implementation head and completed the Release/CUDA build.

Observed result:

```text
11/12 CTests PASS
air-reference-tests FAIL
air-cuda-contract-tests PASS
```

The sole failure was the destructive test for a missing required semantic role.

Root cause:

`validate_prepared_model_semantics()` checked descriptor ownership before required-role presence. A null required binding was therefore classified as `invalid_argument` (foreign/unowned descriptor) instead of the intended `data_error` (missing required semantic role).

This was a validator error-classification defect, not a numerical or architecture-boundary failure.

Fix:

- ownership validation now applies only to descriptors that are actually bound;
- missing required roles continue to the role/shape validator and fail as `data_error`;
- non-null descriptors from another canonical model still fail as `invalid_argument`;
- the falsification test expectation was not weakened.

Wave 6 remains open until the fixed exact head passes the complete qualification suite.

## Wave 6 exit gate

Wave 6 may close only when the exact branch HEAD on WolfCat-Studio:

1. builds in Release mode with CUDA enabled;
2. passes the complete 12-test CTest suite;
3. passes renamed-source Reference execution parity;
4. passes destructive prepared-semantic validation tests;
5. passes renamed-source CUDA execution parity on the NVIDIA device;
6. passes renamed-source dense-FP32 tactic preparation and execution;
7. preserves public rejection of renamed Qwen2 source tensors;
8. keeps Reference/CUDA free of hard-coded canonical tensor source names;
9. requires no MEF, Builder, scheduler, or serving compensation.

If this passes, the modular model-architecture boundary has survived its intended R0 falsification and Wave 7 may begin full-system release qualification.
