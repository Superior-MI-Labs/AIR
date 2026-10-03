# AIR Public Contracts

This document defines the AIR 0.11.0 compatibility boundary. AIR is pre-1.0;
binary ABI stability is not promised.

## Installed executables

- `air-cli`
- `air-server`
- `air-bench`
- `air-qualify`
- `air-verify`
- `air-strategy-probe`

## C++ package

Installed CMake exports:

```cmake
find_package(AIR CONFIG REQUIRED)
target_link_libraries(app PRIVATE AIR::core AIR::cuda)
```

The primary embedding authority remains `air::InferenceService`.

Low-level executors, graph internals, prepared backend state, semantic extension
internals, and evidence structures are expert/research surfaces and may evolve
before 1.0.

## HTTP contract

Read-only surfaces:

```text
GET /
GET /health
GET /model
GET /runtime
GET /machine
GET /environment
GET /events
GET /timeline
GET /execution-graphs
GET /semantics
GET /metrics
GET /v1/models
```

Mutation/work surfaces:

```text
POST /generate
POST /v1/completions
POST /v1/chat/completions
POST /decide
```

### Native generation

`/generate` accepts exactly:

```text
prompt: string, required
max_tokens: non-negative integer
temperature: finite number >= 0
top_p: finite number in (0,1]
top_k: non-negative integer
seed: non-negative uint64
stream: boolean
```

An unknown native AIR field is an invalid request and maps to HTTP 400.

### OpenAI-shaped compatibility routes

Completion/chat routes additionally accept `model` as an optional single-model
compatibility field, `n` only when equal to 1, and either `max_tokens` or
`max_completion_tokens` but not both.

Chat message objects accept exactly:

```text
role: string
content: string
```

Recognized compatibility capabilities that AIR does not implement remain
explicit unsupported errors and map to HTTP 501 rather than being silently
approximated.

`model` does not perform model routing.

### Decision

`POST /decide` is a bounded semantic candidate-scoring request. Returned
normalized scores are candidate-set-relative scores, not calibrated confidence
or probability.

## Adaptive read-only contracts

### Machine

`/machine` is the stable structural topology view.

`/environment` is dynamic availability/observation state and references the
topology fingerprint. Volatile free-memory or utilization changes do not mutate
topology identity.

### Timeline

Execution observation uses a bounded timeline. Non-zero work counts are typed.
Qualified work-unit kinds are:

- `tokens`
- `iterations`
- `bytes`

Zero-work administrative/synchronization spans may be unitless.

### ExecutionGraph

ExecutionGraph R1 schema version is `3`.

Graphs distinguish:

- workload kind;
- physical invocation;
- optional autoregressive state;
- typed workload work;
- auxiliary item multiplicity;
- placement;
- dependencies;
- implementation/resource references;
- opaque semantic value identities;
- descriptive versus AIR-executable binding.

Generic graph code must not infer semantic meaning from opaque IDs.

### Semantics

`/semantics` exposes the trusted implementation registry and structured
missing-semantic state.

Packages are data-only. They cannot provide executable scripts, commands,
entrypoints, library paths, or arbitrary source code.

## Evidence schemas retained from 0.10

### Execution manifest

`schema_version = 10`

Evidence manifests are not durable configuration. Stale/foreign schema
identities are rejected and requalification is the migration mechanism.

### Benchmark report

`air.benchmark.v11`

### Verification report

`air.verification.v1`

## Capability reporting

A capability is reported only when the selected backend/implementation can
actually provide it.

In particular:

- scheduler quantum is not native backend width;
- KV page size is physical storage geometry;
- prepared-resource byte equality does not imply resource identity;
- descriptive FLUX graphs are not executable image-generation support;
- a resolved semantic registry entry is distinct from a descriptive package
  requirement;
- retained earlier CUDA evidence is distinct from final-source CUDA
  qualification.

## Compatibility policy

Before 1.0:

- correctness/security fixes may change erroneous behavior;
- additive diagnostic fields may be introduced with schema advancement;
- unsupported inputs fail explicitly;
- frozen evidence schemas are never silently reinterpreted;
- architecture expansion enters through explicit semantic/implementation
  contracts, not hidden fallback paths.
