# AIR 0.10.0 — Modular Model Architecture Boundary

AIR 0.10.0 is the second public R&D release of the Adaptive Inference Runtime.

The release changes where model-family knowledge lives inside AIR without
replacing AIR's production inference pipeline.

## What changed

AIR now has an explicit internal model-architecture preparation boundary:

```text
GGUF
  -> ModelDefinition
  -> Architecture Adapter
  -> PreparedModelSemantics
  -> Reference / CUDA
```

The qualified Qwen2 adapter resolves source tensor names, geometry, optional
biases, tied output, and current Qwen2 semantic restrictions once.

Reference and CUDA execution consume those semantic bindings rather than
reconstructing Qwen2/GGUF tensor names in their numerical execution paths.

CUDA prepared-linear tactic eligibility also derives from semantic roles rather
than Qwen2 source-name patterns.

## Why it matters

The runtime no longer requires its executors to know how a qualified model
family names its source tensors.

That creates a clean future compiler/runtime boundary:

```text
future declarative model representation
            |
            v
prepared semantic model contract
            |
            v
           AIR
        /       \
   Reference   CUDA
```

AIR remains the execution authority. Builder/Neural Model IR work remains a
separate program.

## Falsification

The boundary was attacked with test-only models whose tensor bytes and Qwen2
computation remained valid while canonical source names were replaced with
unrelated identifiers.

The qualification proves that:

- renamed-source Reference execution preserves numerical output;
- renamed-source CUDA execution preserves numerical output;
- CUDA dense-FP32 prepared tactic selection works from semantic roles;
- public Qwen2 loading does not silently accept those aliases;
- missing, foreign, duplicated, malformed, or semantically unsupported
  prepared bindings fail explicitly.

## Existing system contracts preserved

AIR 0.10.0 preserves the qualified production surfaces for:

- bounded admission and HTTP 503 backpressure;
- cancellation and deterministic resource reclamation;
- sequence-state ownership;
- Reference and NVIDIA CUDA execution;
- native `/generate`;
- OpenAI-shaped `/v1/completions` and `/v1/chat/completions`;
- native `/decide`;
- runtime/model/event/metrics observability;
- AIR Web 3.2;
- installed CMake consumers;
- MEF R0 `provider.air.http` compatibility.

## Qualified model scope

AIR 0.10.0 production model support remains intentionally narrow:

- GGUF v3;
- `qwen2` architecture metadata;
- qualified Qwen2.5 GGUF path;
- GPT-2 BPE / Qwen2 pre-tokenization;
- Qwen2 ChatML;
- grouped-query attention;
- optional Q/K/V biases;
- RMSNorm;
- SwiGLU;
- full even head-dimension RoPE;
- tied output embeddings.

Unsupported model semantics still fail explicitly.

## Not claimed

AIR 0.10.0 does not claim:

- universal GGUF execution;
- arbitrary neural architectures;
- a public universal Neural Model IR;
- Builder-to-AIR arbitrary model compilation;
- additional qualified production model families.

Those are later programs that can consume the boundary introduced here.

## Qualification

The release candidate is required to pass:

- all 12 CTests in Release/CUDA configuration;
- renamed-source semantic falsification;
- frozen-v0.9.12 same-machine differential non-regression;
- public HTTP/browser/generation/Decision checks;
- malformed protocol rejection;
- stress, cancellation, shutdown/restart, overload, and reclamation gates;
- external frozen MEF R0 provider-substitution qualification.

See:

- `docs/modular-runtime/WAVE7-QUALIFICATION.md`
- `docs/modular-runtime/WAVE8-RELEASE.md`
- `docs/RELEASE-PROVENANCE.md`
- `docs/SUPPORT_MATRIX.md`
