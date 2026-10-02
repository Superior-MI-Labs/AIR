# Prompt 11 - Hosted RC Hardening and Evidence Debt

Status: CURRENT
Program: AIR 0.11.0 Adaptive Execution Foundation

## Situation

The primary development laptop is unavailable because of a hardware
malfunction.

Prompt 11 therefore distinguishes:

1. hosted qualification that can be reproduced in GitHub Actions;
2. retained earlier WolfCat evidence;
3. post-architecture hardware/user evidence that is still unavailable.

The missing hardware is not converted into a passing result.

## Existing contract coverage

The C++ suite already characterizes:

- bounded scheduler admission;
- overload/backpressure;
- generation cancellation;
- queued cancellation;
- shutdown cancellation;
- Decision branch cancellation;
- mixed generation/Decision fairness;
- stream-delivery isolation;
- repeated service lifecycle;
- logical resource reclamation;
- malformed protocol handling;
- machine topology fixtures;
- semantic extension security.

## Hosted installed-product test

`scripts/qualify-adaptive-prompt11-hosted.sh` uses the already-built
CPU/reference configuration and:

1. installs AIR into an isolated prefix;
2. generates the repository's tiny Qwen2 GGUF fixture;
3. builds/runs an external `find_package(AIR)` consumer;
4. starts the installed `air-server` in reference mode;
5. verifies the installed Web 3.3 application;
6. checks the canonical read-only endpoints;
7. validates machine/environment fingerprint agreement;
8. validates /semantics resolved/missing truth;
9. performs real HTTP generation;
10. performs real HTTP Decision;
11. verifies unsupported request fields fail explicitly;
12. reads detailed timeline and ExecutionGraph R1 evidence;
13. shuts down and restarts the installed server;
14. verifies semantic registry state after restart;
15. proves no installed server process is left behind;
16. retains checksummed hosted evidence.

## Claims allowed from this harness

If it passes, AIR may claim:

- hosted CPU/reference install and server integration qualified;
- installed Control Room asset delivery qualified;
- HTTP generation/Decision smoke paths qualified;
- machine/environment/semantics/read-only surfaces qualified in hosted mode;
- ExecutionGraph R1 reference observation qualified;
- restart/reconnect server-side behavior qualified;
- external CMake package consumption qualified.

It may not claim:

- post-R1 CUDA graph concordance;
- NVIDIA memory/thermal/pressure behavior;
- AIR-owned FLUX model component execution;
- FLUX image parity;
- human novice/research usability.

## Hardware/user evidence debt

Pending because the development hardware is unavailable:

- real post-R1 Qwen CUDA graph/evidence replay;
- real CUDA prepared-resource pressure/transition replay on final source;
- FLUX text-encoder/denoiser/VAE AIR-owned execution;
- FLUX decoded pixel parity against the retained oracle;
- thermal/power drift on target NVIDIA hardware;
- interactive human Control Room usability session;
- real browser disconnect/reconnect observation on the development machine.

These are release limitations, not hidden passes.

## Exit

Prompt 11 closes when:

- hosted installed-product qualification passes exact source;
- all canonical CPU tests pass;
- Control Room web preflight passes;
- defects discovered by hosted hardening are fixed at owning seams;
- remaining unavailable evidence is recorded explicitly for Prompt 12.
