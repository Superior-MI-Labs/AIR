# AIR Public Contracts

This document defines the AIR 0.10.0 compatibility boundary. It separates supported user-facing contracts from diagnostic/internal implementation surfaces.

AIR is pre-1.0. Binary ABI stability is not promised. Within the 0.10 release line, source/API and wire-format changes should remain explicit and evidence-backed; correctness or security fixes may change erroneous behavior.

## Stable 0.10.0 surfaces

### Executables

- `air-cli`
- `air-server`
- `air-bench`
- `air-qualify`
- `air-verify`

### Primary C++ service surface

`air::InferenceService` and its request/response/cancellation types are the supported embedding boundary:

- `InferenceRequest`
- `InferenceResponse`
- `GenerationConfig`
- `SamplingConfig`
- `RequestMetrics`
- `CancellationSource` / `CancellationToken`
- `ServiceSnapshot`
- `SchedulerConfig`
- `ManifestConfig`

`ModelDefinition`, tokenizer/model metadata, status/result types, and execution-plan data are also public because they appear transitively in supported interfaces.

Low-level `ReferenceExecutor`, `CudaExecutor`, concrete KV-cache classes, and verification-stage objects are expert/diagnostic APIs. They are installed and tested, but they do not carry source-compatibility promises before 1.0.

### CMake package

Installed AIR exports:

```cmake
AIR::core
AIR::cuda
```

through `find_package(AIR CONFIG REQUIRED)`.

## HTTP contract

Endpoints:

```text
GET  /
GET  /health
GET  /model
GET  /runtime
GET  /events
GET  /metrics
GET  /v1/models
POST /generate
POST /v1/completions
POST /v1/chat/completions
POST /decide
```

`/generate` accepts:

```text
prompt: string, required
max_tokens: non-negative integer
temperature: finite number >= 0
top_p: finite number in (0,1]
top_k: non-negative integer
seed: non-negative uint64
stream: boolean
```

OpenAI-shaped completion endpoints additionally accept `model` as an optional string compatibility field, `n` only when equal to 1, and either `max_tokens` or `max_completion_tokens` but not both.

Chat message objects accept exactly:

```text
role: string
content: string
```

Unknown fields are rejected with an explicit unsupported error. Unsupported protocol features are never silently approximated.

`model` is accepted for single-model client compatibility; it does not perform model routing.

## Frozen evidence schemas

### Execution manifest

`schema_version = 10`

AIR reads and writes manifest schema v10 only. Older schema identities are
rejected rather than silently reinterpreted. Requalification is the migration
mechanism because manifests represent measurement evidence, not durable user
configuration.

### Benchmark report

`air.benchmark.v11`

Percentiles use shared linear interpolation semantics.

### Verification report

`air.verification.v1`

Teacher-forced reference/CUDA comparison and optional stage evidence are diagnostic outputs, not performance results.

## Internal model-architecture boundary

AIR 0.10.0 introduces an internal architecture-adapter and prepared-semantic
model boundary.

This boundary is intentionally not a new stable public Neural Model IR.
`ModelDefinition` remains canonical model truth. Architecture-specific
preparation resolves qualified source tensors and semantic constraints into
derived execution bindings. Low-level prepared semantic types and internal
executor factories remain implementation details.

Qwen2 remains the only qualified production model architecture in 0.10.0.

## Capability semantics

A capability is reported only when the selected prepared backend implements it physically.

In particular:

- scheduler prefill quantum is not backend native batch width;
- KV page size is physical storage geometry;
- checkpoint support does not imply persistent prefix reuse;
- admitted concurrency does not imply fused multi-sequence CUDA decode;
- automatic planning may select only plans valid for the prepared backend's capabilities.

## Compatibility policy

Before 1.0:

- correctness fixes may change erroneous behavior;
- new optional fields may be added;
- unsupported inputs remain explicit failures rather than best-effort guesses;
- existing frozen evidence schemas are not silently reinterpreted;
- architecture expansion belongs to a new documented capability, not a hidden fallback path.
