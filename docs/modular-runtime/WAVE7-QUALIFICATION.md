# AIR Modular Runtime R0 - Wave 7 Full-System Qualification

Status: IMPLEMENTED / AWAITING MACHINE QUALIFICATION

## Objective

Qualify the modular model-architecture boundary as part of the complete AIR system without broadening the architecture.

No production architecture changes are authorized in this wave unless qualification demonstrates a correctness or release-safety defect.

## Qualified input from Wave 6

Wave 6 closed with:

```text
Release build
CUDA=ON
12/12 CTests PASS
0 failures
```

That result includes:

- renamed-source Reference execution parity;
- renamed-source CUDA execution parity;
- dense-FP32 tactic preparation from semantic roles;
- destructive prepared-semantic validation;
- preservation of public Qwen2 source-name requirements.

## Release-qualification source drift discovered

The historical release-candidate harness and public documentation contain schema identities from earlier AIR development phases.

Live source at the Wave 7 branch defines:

```text
execution manifest schema: 10
benchmark report schema:    air.benchmark.v11
verification report schema: air.verification.v1
```

Historical/current narrative files still contain older schema numbers.

Wave 7 does not reinterpret old frozen evidence.

The machine qualification harness now derives current schema identities from source rather than hard-coding a historical schema.

Public documentation is updated consistently only during the final Wave 8 release freeze.

## Isolation rule

Wave 7 must not qualify whatever AIR happens to be installed in `~/.local`.

The qualification entrypoint:

```text
scripts/qualify-modular-runtime-r0.sh
```

requires:

- the exact `architecture/modular-runtime-r0` branch;
- a clean source worktree;
- a user-supplied supported GGUF model;
- a new isolated Release/CUDA build directory;
- a new isolated install prefix;
- all executed AIR binaries to resolve from that isolated prefix.

The exact Git HEAD, model SHA-256, model size, build path, prefix, RC archive path, and RC archive SHA-256 are retained in the Wave 7 evidence directory.

## Qualification program

The Wave 7 machine program performs the following gates.

### Build and package

- Release build;
- CUDA enabled;
- complete CTest suite;
- install into isolated prefix;
- installed binary origin verification;
- external `find_package(AIR)` consumer configure/build/run.

### Numerical execution

- Reference/CUDA differential verification on the real model;
- fresh CUDA-only qualification manifest;
- current manifest schema identity derived from source;
- previous manifest schema explicitly rejected.

### Public surfaces

- executable help/version surfaces;
- `GET /`;
- `GET /health`;
- `GET /model`;
- `GET /runtime`;
- `GET /events`;
- `GET /metrics`;
- `GET /v1/models`;
- native `POST /generate`;
- `POST /v1/completions`;
- `POST /v1/chat/completions`;
- native `POST /decide`;
- unsupported-field rejection;
- conflicting-token-field rejection.

### Browser and generation

The existing AIR web doctor is run against both:

- the exact source web tree;
- the isolated installed web tree;
- the live served application.

The existing generation doctor validates:

- non-streaming generation response text;
- streaming SSE delivery.

### Reliability and lifecycle

- concurrent generation stress;
- malformed/invalid request rejection;
- client disconnect/cancellation;
- idle resource reclamation;
- clean server shutdown;
- fresh server restart;
- deterministic one-active/one-queued overload configuration;
- synchronized request pressure;
- at least one HTTP 503 rejection;
- runtime overload counter increment;
- post-overload resource reclamation;
- clean overload-server shutdown.

## MEF compatibility is external

AIR does not own MEF qualification.

After the AIR-local Wave 7 program passes, frozen MEF R0 remains an independent external compatibility authority.

The AIR endpoint must still satisfy the frozen `provider.air.http` contract for:

```text
text.generate@1.0.0
```

using MEF tag:

```text
mef-r0-qualified
33f63246244f91acfba5659bba80447e6e181108
```

The MEF gate must not require changes to MEF provider/core semantics solely because AIR internals became modular.

## Evidence location

The qualification entrypoint creates:

```text
~/Downloads/AIR-Modular-Runtime-W7-<timestamp>/
```

containing, at minimum:

- `identity.txt`;
- isolated build tree;
- isolated install prefix;
- RC evidence directory;
- RC evidence ZIP;
- `wave7-summary.txt`.

The RC evidence includes per-gate exit codes and generated runtime artifacts.

## Wave 7 exit gate

Wave 7 may close only when:

1. the exact clean branch head completes the isolated Release/CUDA build;
2. all 12 CTests pass;
3. the machine RC validator reports `overall_fail=0`;
4. Reference/CUDA verification passes on the real supported model;
5. installed-package consumer passes;
6. generation, chat, Decision, browser, and observability endpoints pass;
7. malformed/unsupported protocol behavior remains explicit;
8. normal stress and disconnect tests reclaim resources;
9. bounded overload produces explicit HTTP 503 evidence and reclaims resources;
10. shutdown and restart are clean;
11. frozen MEF R0 independently requalifies AIR as `provider.air.http`;
12. no Builder, MEF, scheduler, serving, or public capability redesign is required.

If a gate fails, repair the owning defect and rerun the affected qualification. Do not broaden the architecture to make qualification pass.

If all gates pass, Wave 8 may freeze the exact release source, update the public documentation/version surface, rerun final identity/smoke checks, and publish AIR 0.10.0.
