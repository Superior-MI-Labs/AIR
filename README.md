<p align="center">
  <img src="docs/assets/air-github-banner.svg" width="100%" alt="AIR — Adaptive Inference Runtime by Superior MI Labs">
</p>

<h1 align="center">AIR</h1>

<p align="center">
  <strong>Adaptive Inference Runtime</strong><br>
  Local-first execution with explicit semantics, machine-aware planning,
  bounded runtime ownership, and falsifiable evidence.
</p>

<p align="center">
  <a href="https://github.com/Superior-MI-Labs/AIR/releases/tag/v0.11.0">
    <img src="https://img.shields.io/badge/release-v0.11.0-55d9ff?style=for-the-badge&labelColor=07131d" alt="AIR 0.11.0">
  </a>
  <img src="https://img.shields.io/badge/C%2B%2B-20-55d9ff?style=for-the-badge&labelColor=07131d" alt="C++20">
  <img src="https://img.shields.io/badge/CTest-19%2F19%20PASS-42c98b?style=for-the-badge&labelColor=07131d" alt="19 of 19 hosted CTests passing">
  <img src="https://img.shields.io/badge/Web-3.3-76dfff?style=for-the-badge&labelColor=07131d" alt="AIR Web 3.3">
  <a href="LICENSE">
    <img src="https://img.shields.io/badge/license-Apache%202.0-e7a85f?style=for-the-badge&labelColor=07131d" alt="Apache License 2.0">
  </a>
</p>

**AIR 0.11.0 — Adaptive Execution Foundation** evolves AIR from a
Qwen2-oriented inference runtime toward a machine-aware computation runtime
without introducing a second scheduler, planner, or model-family engine.

AIR remains public research software. Release claims are intentionally narrower
than the architecture's long-term goal.

## What 0.11 adds

AIR 0.11 introduces:

- canonical hardware topology and dynamic execution-environment snapshots;
- typed execution observation and bounded timelines;
- explicit semantic-operation versus physical-implementation boundaries;
- ExecutionGraph R1 with workload-scoped invocation and typed work units;
- evidence-backed adaptive strategy planning;
- identified prepared-resource residency instead of anonymous byte equivalence;
- explicit work-unit identities for tokens, iterations, and bytes;
- a second structural workload, the qualified FLUX.2 Klein oracle;
- shared Qwen/FLUX workload, resource, invocation, and graph abstractions;
- AIR-owned deterministic FLUX semantic operations for latent geometry and
  sigma-schedule derivation;
- structured missing-semantic resolution through one trusted implementation
  registry;
- AIR Control Room Web 3.3;
- hosted installed-product qualification for the reference path.

The high-level direction is:

```text
package / immutable resources
            ↓
semantic computation
            ↓
hardware topology + environment + evidence
            ↓
physical execution plan / ExecutionGraph
            ↓
one AIR runtime
            ↓
measured execution
```

AIR may aggressively adapt **how** known computation executes. It must not guess
**what** unknown computation means.

## Qualified execution scope

### Qwen2

Qwen2 remains the only production model family with an AIR-owned end-to-end
inference executor.

Qualified functionality includes the Reference path and retained NVIDIA CUDA
evidence from the adaptive-execution program. The final 0.11 source is
hosted-qualified on the Reference path.

Because the development NVIDIA machine became unavailable during final RC
hardening, **post-ExecutionGraph-R1 CUDA replay on the final exact 0.11 source is
not claimed as qualified**. The CUDA backend remains available, but that exact
hardware gate is recorded as release evidence debt.

### FLUX.2 Klein

FLUX.2 is the second architecture discriminator, not a claimed production image
backend.

AIR 0.11 qualifies:

- the frozen external FLUX.2 Klein semantic oracle contract;
- semantic value identities and operation ordering;
- iterative physical invocation structure;
- text-encoder / denoiser / VAE resource identities;
- shared component/resource planning structures;
- descriptive ExecutionGraph R1 projection;
- AIR-owned deterministic latent-geometry and sigma-schedule semantics;
- structured reporting of the nine still-missing model/tensor/component
  semantic implementations.

AIR 0.11 does **not** claim AIR-owned text-encoder, denoiser, VAE execution or
decoded-image parity.

## Control Room

AIR Web 3.3 projects canonical server state. It does not own runtime truth.

The Control Room consumes:

```text
/health
/model
/runtime
/machine
/environment
/events
/timeline
/execution-graphs
/semantics
/metrics
```

It exposes machine state, workload/runtime state, execution evidence,
ExecutionGraph structure, resource residency, and unsupported semantic
requirements.

## Public HTTP surface

```text
GET  /
GET  /health
GET  /model
GET  /runtime
GET  /machine
GET  /environment
GET  /events
GET  /timeline
GET  /execution-graphs
GET  /semantics
GET  /metrics
GET  /v1/models

POST /generate
POST /v1/completions
POST /v1/chat/completions
POST /decide
```

The native AIR request schema treats unknown fields as invalid requests.
OpenAI-shaped compatibility routes keep recognized-but-unimplemented
capabilities explicit rather than silently approximating them.

## Quick start on Linux

```bash
git clone https://github.com/Superior-MI-Labs/AIR.git
cd AIR

./bootstrap.sh
./scripts/build.sh
ctest --test-dir build --output-on-failure
./scripts/install-local.sh
```

Start AIR with a qualified Qwen2-family GGUF:

```bash
./run-air.sh /path/to/model.gguf
```

Then open:

```text
http://127.0.0.1:8181
```

Required base toolchain:

- C++20 compiler;
- CMake 3.22+;
- pkg-config;
- PCRE2 8-bit development headers;
- Boost headers including Beast and JSON.

CUDA remains optional and requires a compatible NVIDIA driver/toolkit and
cuBLAS.

AIR is a direct runtime. **llama.cpp and ComfyUI are not AIR runtime
dependencies.**

## Architecture rules

AIR's current design rules include:

- one canonical state owner per domain;
- one production runtime and scheduler path;
- semantic identity separate from physical execution identity;
- stable hardware topology separate from dynamic environment state;
- raw observations separate from inference and policy;
- explicit resource identity instead of byte-equivalence guesses;
- the browser is a read-only/thin control surface;
- missing semantics fail structurally;
- packages cannot inject arbitrary executable code;
- optimizations require retained evidence;
- negative experiments remain evidence.

See:

- `docs/ARCHITECTURE.md`
- `docs/SUPPORT_MATRIX.md`
- `docs/PUBLIC_CONTRACTS.md`
- `docs/RELEASE-0.11.0.md`
- `docs/adaptive-execution/RELEASE-0.11-STRATEGY.md`

## 0.11 hosted qualification

The final hosted release gate requires:

- Web 3.3 preflight;
- 19/19 CTests;
- isolated install;
- external `find_package(AIR)` consumer;
- installed Reference server;
- machine/environment authority checks;
- semantic-registry resolved/missing checks;
- real HTTP generation and Decision;
- invalid native-request rejection;
- detailed timeline and ExecutionGraph R1 observation;
- shutdown/restart;
- process cleanup;
- deterministic source/evidence packaging and SHA-256 manifest.

Hardware/user limitations remain explicit in the release manifest.

## Status and license

AIR is an early-stage Superior MI Labs R&D project. Testing, bug reports,
technical criticism, and contributions are welcome.

The public repository is licensed under the **Apache License 2.0**. See
`LICENSE`, `NOTICE`, and `OPEN_SOURCE_SCOPE.md`.
