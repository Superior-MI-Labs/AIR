# AIR 0.11.0 — Adaptive Execution Foundation

AIR 0.11.0 is the third public R&D release of the Adaptive Inference Runtime.

The release changes AIR from a runtime that primarily knew how to execute one
qualified autoregressive family into a runtime with explicit workload,
resource, machine, execution-graph, and semantic-extension boundaries that
survived a structurally different image workflow.

## Main result

The common architecture now supports both:

- Qwen2 autoregressive token execution;
- the qualified FLUX.2 Klein workflow as a descriptive/partially executable
  semantic and physical structure.

This did **not** require a parallel diffusion runtime, image scheduler, image
planner, or image-specific residency authority.

## Major additions

- HardwareTopology and ExecutionEnvironment authority split.
- Typed execution observation with bounded timelines.
- Semantic-operation versus implementation legality boundary.
- ExecutionGraph R1.
- Evidence-backed adaptive planning.
- Identified prepared-resource residency.
- Explicit work units: tokens, iterations, bytes.
- Workload-scoped autoregressive versus iterative physical invocation.
- FLUX.2 Klein semantic adapter and component/resource plan.
- AIR-owned latent-geometry and sigma-schedule semantics.
- Trusted semantic implementation registry and MissingSemantic reporting.
- `GET /semantics`.
- AIR Control Room Web 3.3.
- Hosted installed-product RC hardening.

## Evidence-backed optimization retained from the program

Prompt 6 showed a real Qwen CUDA bottleneck reduction using prepared dense-FP32
linear state. The experiment retained both positive and negative results and
qualified resource preparation/eviction behavior before later resource
identity generalization.

## FLUX.2 scope

The external oracle established:

- 1024 x 1024, batch 1;
- seed `432262096973490`;
- four Euler sampling transitions;
- CFG 1;
- latent semantic geometry `[1,128,64,64]`;
- exact five-value sigma path;
- exact text-encoder, denoiser, and VAE artifact identities.

AIR 0.11 can represent that workload through the same workload/resource/graph
architecture and executes its deterministic latent-geometry and schedule
semantics internally.

AIR does not yet execute the FLUX model components or claim decoded image parity.

## Hosted exact-source qualification

The final hosted RC gate requires:

- Web 3.3 preflight;
- 19/19 CTests;
- isolated install;
- external CMake consumer;
- installed reference server;
- canonical machine/environment/semantics surfaces;
- generation and Decision over HTTP;
- malformed native-request handling;
- detailed timeline and ExecutionGraph R1 observation;
- shutdown/restart and registry restart;
- deterministic source/evidence archives and SHA-256 manifest.

## Release limitations

The development NVIDIA laptop became unavailable during final RC hardening.

Therefore this release does not claim:

- post-R1 Qwen CUDA graph concordance on the final exact 0.11 source;
- final-source NVIDIA memory/thermal/power qualification;
- AIR-owned FLUX text-encoder/denoiser/VAE execution;
- FLUX image parity;
- final human novice/research Control Room usability.

Earlier Prompt 8B/8C machine evidence remains valuable retained evidence, but it
is not silently promoted into final-source qualification.

## Compatibility

AIR remains pre-1.0. Unsupported semantics and protocol behavior fail
explicitly rather than being guessed or approximated.

See `docs/PUBLIC_CONTRACTS.md`, `docs/SUPPORT_MATRIX.md`, and
`docs/RELEASE-PROVENANCE.md`.
