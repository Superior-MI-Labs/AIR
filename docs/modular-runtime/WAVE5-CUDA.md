# AIR Modular Runtime R0 - Wave 5 CUDA Migration

Status: CLOSED / QUALIFIED

## Objective

Make every qualified CUDA numerical execution path consume the same prepared semantic model contract used by Reference.

This wave changes tensor addressing and model-geometry sourcing. It does not redesign CUDA kernels, KV paging, workspaces, tactics, batching, admission, scheduling, or device-memory ownership.

## Prepared CUDA authority

`CudaExecutor::Impl` now retains one `PreparedModelSemantics` produced during initialization.

CUDA derives:

- validated geometry;
- head dimension;
- token embedding;
- output normalization;
- explicit tied/untied output weight;
- optional output bias;
- per-layer normalization;
- Q/K/V weights;
- optional Q/K/V biases;
- attention output;
- FFN gate/up/down bindings;

from that prepared model.

`ModelDefinition` remains canonical source truth. The prepared model remains derived state.

## Resident tensor ownership

The existing CUDA model arena remains the physical tensor owner.

Semantic bindings point to canonical `TensorDescriptor` objects.

CUDA resolves those descriptors to the already-resident physical tensor through descriptor identity/source identity. This does not create a second model arena or tensor store.

## Migrated execution paths

All currently qualified CUDA neural paths now use semantic bindings:

1. single-token decode;
2. native batched decode;
3. single-sequence native prefill;
4. multi-sequence native prefill.

Tied output is no longer rediscovered through `find_tensor("output.weight")`.

Optional Q/K/V and output biases are supplied through semantic optional bindings rather than reconstructed source names.

## Execution-strategy preparation

The optional dense-FP32 and Q5/Q8 DP4A prepared-linear tactics previously identified transformer block matrices by Qwen2 source-name patterns.

That hidden dependency has been removed.

Block-linear tactic eligibility is now derived from semantic layer roles:

- query;
- key;
- value;
- attention output;
- FFN gate;
- FFN up;
- FFN down.

The tactic caches may still use each tensor's source descriptor name as an internal physical lookup key. That is provenance/identity, not a hard-coded model-family naming rule.

## Source-level boundary

At the Wave 5 implementation head, `src/cuda/cuda_backend.cu` contains no hard-coded occurrences of:

```text
blk.
token_embd.weight
output_norm.weight
output.weight
attn_q.weight
attn_q.bias
Qwen2
qwen2
```

and contains no `ModelDefinition::find_tensor()` call.

CUDA execution geometry is sourced from `PreparedModelSemantics::geometry` and `head_dimension`.

## Numerical qualification

The existing `air-cuda-contract-tests` target has been strengthened rather than creating another test executable.

When a CUDA device is available, it now builds one tiny valid Qwen2 model and compares CUDA against Reference across:

- single-token decode logits;
- single-sequence prefill logits;
- native batched-decode greedy outputs;
- multi-sequence prefill logits.

CPU-only/no-device behavior keeps the existing explicit skip/unsupported contract.

This turns the existing CUDA CTest into a real semantic-binding execution qualification on WolfCat-Studio.

## Intentionally unchanged

- CUDA kernels;
- cuBLAS use;
- quantized kernels;
- workspace allocation model;
- model arena ownership;
- KV paging/pools;
- SequenceState behavior;
- decode/prefill batch geometry;
- execution tactic policy;
- scheduler/admission;
- Reference math;
- serving/Decision/HTTP/browser;
- public AIR API;
- MEF R0;
- Builder.

## Wave 5 qualification result

WolfCat-Studio qualification reported:

```text
Release build
CUDA=ON
12/12 CTests PASS
0 failures
```

The strengthened CUDA contract test therefore executed on the actual NVIDIA device and passed the semantic-binding migration together with the existing system suite.

Wave 5 is CLOSED / QUALIFIED.

Wave 6 is authorized to falsify the architecture boundary with alternate source tensor identities and destructive prepared-state tests.
