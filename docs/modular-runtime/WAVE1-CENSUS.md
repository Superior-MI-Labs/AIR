# AIR Modular Runtime R0 - Wave 1 Architecture Census

Status: SOURCE CENSUS IN PROGRESS
Source branch: `architecture/modular-runtime-r0`
Source baseline: `f57f860515e5deab8b4c31d190b1f091bbbbb15d`

## Purpose

Identify current model-family assumptions before creating a new architecture seam.

This document classifies existing source behavior. It does not itself authorize a universal model representation or a second execution path.

## Classification vocabulary

- **format**: parsing/import concerns specific to an artifact container or metadata convention;
- **model-family semantics**: rules that define the supported neural computation or geometry;
- **semantic tensor binding**: mapping source tensor identity to execution roles;
- **generic execution**: backend mechanics that can operate after semantics and bindings are resolved;
- **runtime**: scheduling, admission, sequence state, backend lifecycle, and execution plans;
- **serving**: request/response rendering and public protocol behavior.

## 1. GGUF import

Primary source:

`src/formats/gguf/gguf_format.cpp`

Observed behavior:

- reads `general.architecture`;
- builds architecture-scoped metadata keys using the declared architecture string;
- fills `ModelConfig` geometry such as layer count, embedding size, attention/KV heads, context length, RoPE metadata, and RMSNorm epsilon;
- builds tokenizer metadata independently from executor code;
- imports source tensor descriptors without deciding their semantic execution roles;
- creates the canonical `ModelDefinition`.

Classification:

- GGUF document parsing: **format**;
- architecture-prefixed metadata extraction: **format -> canonical model metadata**;
- imported source tensor names: **format identity**, not yet semantic roles.

Finding:

The GGUF reader is not the primary Qwen2 execution coupling. It already preserves the declared architecture rather than requiring `qwen2` at load time.

Do not replace it with an architecture registry in this program.

## 2. Qwen2 structural contract

Primary sources:

```text
src/model/qwen2_contract.hpp
src/model/qwen2_contract.cpp
```

Observed responsibilities:

- requires `config.architecture == "qwen2"`;
- validates head and KV geometry;
- validates Qwen2-supported RoPE semantics;
- rejects sliding-window attention;
- validates RMSNorm and feed-forward geometry;
- defines required and optional source tensor names;
- validates source tensor ranks and shapes;
- enumerates tensors required by Qwen2 execution.

Classification:

- geometry and supported RoPE/attention constraints: **model-family semantics**;
- required/optional logical tensors: **model-family semantics**;
- mapping those roles to names such as `blk.N.attn_q.weight`: **semantic tensor binding**.

Finding:

This is the strongest existing seam for an architecture adapter. It should be evolved rather than duplicated.

## 3. Reference executor

Primary source:

`src/reference/reference_executor.cpp`

Observed direct Qwen2 coupling:

- includes `model/qwen2_contract.hpp`;
- calls `validate_qwen2_structure()`;
- calls `qwen2_execution_tensor_names()`;
- reconstructs `blk.<layer>.*` tensor names during execution;
- directly looks up Q/K/V, attention projection, normalization, and FFN tensors by source name;
- implements the currently qualified transformer semantics: RMSNorm, Q/K/V projection, optional Q/K/V bias, full RoPE, grouped-query causal attention, residuals, SwiGLU, final norm, and output projection/tied output.

Classification:

- source-name reconstruction and lookup: **semantic tensor binding leakage**;
- numerical RMSNorm/attention/SwiGLU implementation: **qualified model-family execution semantics**;
- tensor decoding/matvec mechanics: **generic execution mechanics within the current scope**.

Authorized Wave 3/4 target:

Remove source tensor-name ownership from ReferenceExecutor.

Not authorized yet:

Replace its numerical algorithm with a general graph interpreter.

## 4. CUDA executor

Primary source:

`src/cuda/cuda_backend.cu`

Observed direct Qwen2 coupling:

- includes `model/qwen2_contract.hpp`;
- calls `validate_qwen2_structure()`;
- obtains `qwen2_execution_tensor_names()` during model preparation;
- reconstructs `blk.<layer>.*` names in single decode, batched decode, and prefill paths;
- resolves Q/K/V, norms, output projection, and FFN weights by those source names;
- implements current Qwen2-compatible RoPE, RMSNorm, attention, FFN, KV, and projection execution using CUDA-specific mechanics.

Classification:

- source-name reconstruction and resident tensor lookup: **semantic tensor binding leakage**;
- CUDA kernel selection, workspace, residency, page/KV mechanics, matmul implementation, batching, and device transfer behavior: **generic backend execution mechanics**;
- current sequence of transformer operations: **qualified model-family execution semantics**.

Authorized Wave 3/5 target:

Resolve semantic bindings before hot execution and consume them through the prepared model/backend state.

Not authorized:

Replace CUDA kernels, scheduler, KV ownership, tactics, or execution plans solely to achieve architectural neatness.

## 5. Tokenizer

Primary source:

`src/model/tokenizer.cpp`

Observed behavior:

The tokenizer selects pre-tokenization regex behavior from tokenizer metadata. The `qwen2` pre-tokenizer label shares a pattern with other known labels.

Classification:

**tokenizer semantics**, separate from neural tensor binding.

Finding:

Do not pull tokenizer internals into the model execution architecture adapter unless a concrete later requirement demonstrates that ownership is wrong.

## 6. Chat rendering

Primary source:

`src/runtime/serving.cpp`

Observed behavior:

`InferenceService::render_chat()` explicitly rejects non-`qwen2` model architecture and implements the currently qualified Qwen2 chat rendering behavior.

Classification:

**serving/prompt-rendering semantics**.

Finding:

This is real model-family coupling, but it is not tensor execution coupling.

Branch 1 should preserve the current public behavior. If it is later modularized, it should become a narrow serving/template policy seam rather than being mixed into the prepared neural model contract.

## 7. Runtime layers checked

Sources checked include:

```text
src/runtime/backend.cpp
src/runtime/execution.cpp
```

No direct Qwen2/tensor-name coupling was found in the source census.

Classification:

**runtime/generic execution orchestration**.

Finding:

This supports the existing architectural rule that Branch 1 should not rewrite runtime scheduling or execution-plan authority.

## 8. Tests and characterization

Existing tests already contain explicit Qwen2 fixtures and model construction in:

```text
tests/core_tests.cpp
tests/reference_tests.cpp
tests/serving_tests.cpp
```

The current test suite therefore provides useful characterization of the qualified family.

Wave 1 should add tests only for seams not already protected.

Highest-value characterization gaps to protect before refactoring:

1. architecture mismatch must remain an explicit unsupported failure;
2. missing required semantic tensor must fail before execution;
3. wrong tensor rank/shape must fail before execution;
4. optional Q/K/V biases must preserve current behavior;
5. tied output behavior when `output.weight` is absent must remain unchanged;
6. executor output must remain independent of any future semantic-role representation;
7. chat rendering remains explicitly Qwen2-only until separately generalized.

## 9. Initial dependency map

```text
GGUF source identity
        |
        v
ModelDefinition
        |
        v
Qwen2 structural contract
   |               |
   |               +-- source tensor names / shape rules
   |
   +-- geometry / supported neural semantics
        |
        +--------------------+
        |                    |
        v                    v
ReferenceExecutor       CUDA preparation/execution
        |                    |
        +---------+----------+
                  |
                  v
          runtime/backend contracts
                  |
                  v
         scheduler / serving
```

Target direction for this R0 program:

```text
GGUF
  -> ModelDefinition
  -> Qwen2 architecture adapter
       -> validated semantic bindings
       -> validated model-family geometry
  -> prepared execution state
       -> Reference
       -> CUDA
```

The adapter may initially remain Qwen2-specific. That is intentional.

## 10. Wave 1 provisional conclusion

The first abstraction should not be a general neural graph.

The smallest evidence-supported change is to split the existing Qwen2 contract into two explicit products:

1. **validated Qwen2 model semantics/geometry**;
2. **resolved semantic tensor bindings**.

Reference and CUDA can then consume those products without constructing GGUF/Qwen2 tensor names themselves.

This preserves the current numerical implementation while creating the seam later Neural Model IR or Builder work can target.

## Wave 1 close conditions still outstanding

Before Wave 1 is CLOSED:

- establish a clean machine baseline for the modular-runtime branch;
- confirm characterization coverage for the listed failure/optional-tensor cases;
- add only missing characterization tests;
- run the applicable test suite on that exact branch;
- record the machine evidence.

Do not start Wave 2 implementation before those gates are met.
