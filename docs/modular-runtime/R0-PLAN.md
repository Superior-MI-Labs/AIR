# AIR Modular Runtime R0 Plan

Status: ACTIVE
Target public endpoint: AIR 0.10.0, subject to qualification
Development branch: `architecture/modular-runtime-r0`

## Objective

Separate model-family interpretation and semantic tensor binding from backend execution without replacing AIR's qualified runtime architecture.

The target flow is:

```text
model artifact
  -> format importer
  -> ModelDefinition
  -> architecture adapter
  -> prepared semantic model
  -> existing Reference / CUDA execution
  -> existing runtime / scheduler / serving
```

This program intentionally stops before a universal Neural Model IR or Builder-to-AIR compiler.

## Frozen ownership

The following remain owners during this program:

- `ModelDefinition`: canonical loaded model definition;
- `InferenceService`: service authority;
- capacity and microbatch schedulers: scheduling and admission authority;
- `SequenceState`: per-request backend state contract;
- Reference executor: numerical correctness oracle;
- CUDA executor: optimized NVIDIA execution;
- MEF: external provider qualification and deployment planning;
- Superior MI Builder: external structural authority.

No wave may create a second owner for those responsibilities.

## Wave sequence

### Wave 0 - baseline and guardrails

Record the current source/release identity, authority boundaries, qualified external contracts, known architecture-specific seams, and release endpoint.

Exit gate:

- documentation-only change;
- no runtime behavior changed;
- active development rules and baseline are explicit.

### Wave 1 - architecture-assumption census

Trace every model-family assumption across loading, validation, reference execution, CUDA preparation/execution, tokenizer handling, and serving boundaries.

Classify each assumption as:

- format concern;
- model-family semantic concern;
- semantic tensor-binding concern;
- generic execution concern;
- runtime concern;
- serving concern.

Add characterization tests only where current behavior is unprotected.

Exit gate:

- every material Qwen2-specific assumption is classified;
- no speculative generic execution subsystem has been introduced.

### Wave 2 - architecture adapter boundary

Introduce the smallest architecture-specific interpretation seam supported by the Wave 1 census.

Qwen2 remains the only production implementation.

Unknown architecture identities fail explicitly.

Exit gate:

- Qwen2 structural validation is owned by the adapter seam;
- existing runtime, scheduler, serving, public API, and ModelDefinition contracts remain intact.

### Wave 3 - prepared semantic model contract

Resolve source tensor names and architecture-specific model rules once into immutable semantic bindings and validated geometry.

Source tensor identity must remain available for provenance, but executors should consume semantic roles rather than reconstruct GGUF/Qwen2 names.

Exit gate:

- semantic tensor roles are validated before execution;
- missing, duplicate, malformed, and shape-incompatible bindings fail structurally;
- no universal neural graph IR is introduced.

### Wave 4 - Reference migration

Convert ReferenceExecutor to consume the prepared semantic model contract.

Do not redesign or optimize transformer math.

Exit gate:

- ReferenceExecutor no longer owns Qwen2/GGUF tensor-name construction;
- numerical characterization and token/logit parity pass.

### Wave 5 - CUDA migration

Convert CUDA model preparation/execution to the same prepared semantic model contract.

Do not replace CUDA kernels, KV ownership, scheduling, admission, or tactic machinery.

Exit gate:

- CUDA execution no longer owns Qwen2/GGUF tensor-name construction;
- Reference/CUDA differential qualification passes.

### Wave 6 - abstraction falsification

Attempt to break the boundary.

Use test-only alternate source tensor names or equivalent binding fixtures to prove executors depend on semantic roles, not Qwen2/GGUF naming.

Attack malformed bindings, model geometry, unsupported semantics, optional tensors, and tampered bindings.

Exit gate:

- hidden executor dependence on source tensor names is absent or repaired at the owning seam;
- all destructive cases fail explicitly.

### Wave 7 - integration and release qualification

Qualify AIR as a complete system after modularization.

Required areas include:

- complete CTest suite;
- Reference and CUDA differential behavior;
- generation and Decision;
- bounded admission and backpressure;
- cancellation and cleanup;
- sequence state;
- HTTP and browser contracts;
- downstream CMake consumer;
- shutdown/restart and resource reclamation;
- AIR's frozen MEF R0 provider compatibility.

Exit gate:

- all release qualification gates pass;
- no MEF or Builder contract change is required to compensate for AIR internals.

### Wave 8 - freeze and public release

If and only if Wave 7 passes:

- set the final release version consistently;
- update public README, support matrix, contracts, architecture documentation, and release notes;
- freeze exact qualified source identity;
- produce checksums and release artifacts;
- tag the qualified commit;
- publish the GitHub Release.

The release must state that Qwen2 remains the qualified production architecture.

It must not claim arbitrary GGUF or universal neural-model support.

## Stop line

Branch 1 closes after the qualified 0.10.0 modular model-architecture boundary is released.

Do not extend this branch into:

- additional production model families;
- universal Neural Model IR;
- Builder-to-AIR compilation;
- model training or distillation;
- experimental model surgery;
- Local Intelligence.

Those belong to later programs that consume the qualified boundary.
