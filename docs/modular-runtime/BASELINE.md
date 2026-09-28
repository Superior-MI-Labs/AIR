# AIR Modular Runtime R0 Baseline

Captured: 2026-09-27
Branch base: `f57f860515e5deab8b4c31d190b1f091bbbbb15d`
Frozen public release: `v0.9.12`

## Frozen release authority

AIR 0.9.12 remains the release authority for 0.9.12 claims.

Recorded public identities:

```text
qualified source fingerprint
587928104d2da182d30903daf00b30d1bd45f3115dcb4896bf3ea3bc70f04fb3

qualified release archive SHA-256
78e463df6c3eb4c8f6f550430fe429579cf08a678bc7cb1d047a702a76763c9f
```

The frozen release reported 12/12 CTests passing plus external package, HTTP, hostile-load, overload, malformed-input, lifecycle, resource-reclamation, and archive-integrity qualification.

Those are historical release facts. They do not automatically qualify later source.

## Current public execution boundary

The public runtime flow at the branch base is:

```text
GGUF
  -> ModelDefinition
  -> PreparedModel/backend preparation
  -> InferenceService
  -> CapacityScheduler
  -> MicrobatchScheduler
  -> SequenceState
  -> Reference or CUDA executor
  -> HTTP / Browser / Bench / Qualification
```

Current qualified production model scope is intentionally narrow:

- GGUF v3;
- architecture metadata `qwen2`;
- tested Qwen2.5 GGUF family;
- Qwen2 tokenizer/pre-tokenization and ChatML behavior;
- F32, F16, BF16, Q4_0, Q5_0, Q8_0, Q4_K, and Q6_K execution tensor encodings.

Broader GGUF inspection does not imply executable model support.

## Existing modular seams

The source already separates several concerns:

- `ModelDefinition` holds canonical model metadata, tokenizer definition, tensor descriptors, storage, and fingerprint;
- GGUF loading constructs `ModelDefinition`;
- runtime/backend contracts separate scheduling from backend implementation;
- Reference and CUDA share runtime-facing execution contracts;
- serving does not own concrete KV implementation.

These seams should be evolved rather than bypassed.

## Current architecture-specific leakage

The current source has a concrete Qwen2 semantic contract:

```text
src/model/qwen2_contract.hpp
src/model/qwen2_contract.cpp
```

It validates Qwen2 geometry and source tensor names.

Both execution implementations directly depend on that model-family contract:

```text
src/reference/reference_executor.cpp
src/cuda/cuda_backend.cu
```

At this baseline, both contain Qwen2-specific tensor-role knowledge and source-name assumptions such as:

```text
token_embd.weight
output_norm.weight
blk.<n>.attn_q.weight
blk.<n>.attn_k.weight
blk.<n>.attn_v.weight
blk.<n>.ffn_gate.weight
blk.<n>.ffn_up.weight
blk.<n>.ffn_down.weight
```

This is the primary seam Branch 1 is authorized to improve.

## What must not move

This program does not reopen:

- one canonical `ModelDefinition`;
- scheduler authority;
- sequence-state ownership;
- physical backend/KV ownership;
- Reference as correctness oracle;
- AIR HTTP/service semantics;
- MEF provider-selection authority;
- Builder structural authority.

## External compatibility authority

MEF R0 is CLOSED, QUALIFIED, and FROZEN.

Its qualified substitution proof includes AIR as:

```text
provider.air.http
capability: text.generate@1.0.0
```

Frozen MEF qualification:

```text
commit 33f63246244f91acfba5659bba80447e6e181108
tag    mef-r0-qualified
```

AIR 0.10.0 qualification must preserve this external provider boundary unless a separately demonstrated contract defect requires coordinated change.

## Known documentation drift

`docs/RELEASE_CANDIDATE.md` contains historical release-candidate schema references that predate the later public contract recorded in `docs/PUBLIC_CONTRACTS.md`.

This Wave 0 program records that as historical documentation drift. It does not reinterpret current schemas or rewrite historical evidence opportunistically.

## Wave 0 verification status

Wave 0 is intentionally documentation-only.

No statement in this file claims that the new development branch has been locally rebuilt or requalified merely because its branch base descends from the qualified 0.9.12 lineage.

Before implementation work advances, local verification should establish a fresh development baseline using the repository's real build/test path, for example:

```bash
./scripts/build.sh
ctest --test-dir build --output-on-failure
```

CUDA qualification should be run on compatible NVIDIA hardware when required by the active wave.
