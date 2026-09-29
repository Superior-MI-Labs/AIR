# AIR 0.11.0 Testing and User-Validation Strategy

Status: ACTIVE
Applies to: Adaptive Execution Substrate R0

## Principle

Different evidence answers different questions.

Do not treat unit tests, benchmarks, GUI checks, machine qualification, and user
testing as interchangeable.

## Layer 1 - Contract/unit tests

Purpose:
Prove deterministic local semantics.

Examples:

- topology validation;
- environment/topology identity binding;
- execution-graph validation;
- implementation legality;
- missing-semantic classification;
- plan identity;
- state ownership;
- serialization/schema validation.

These tests should be fast, deterministic, and model/hardware independent where
possible.

## Layer 2 - Synthetic architecture fixtures

Purpose:
Attack assumptions AIR cannot reproduce on one development machine.

Required synthetic system shapes include:

- CPU-only, high RAM;
- laptop discrete GPU;
- desktop large-VRAM GPU;
- unified-memory SoC;
- dual GPU over ordinary host links;
- dual GPU with fast peer links;
- 8-GPU/datacenter-like topology;
- storage-capacity/offload pressure;
- partial/unknown observations.

A synthetic fixture proves structural handling, not performance.

## Layer 3 - Live machine qualification

Purpose:
Verify AIR's observation and execution claims on real hardware.

Primary development machine:
WolfCat-Studio.

Retain:

- exact source HEAD;
- kernel/OS/toolchain;
- CPU/RAM topology;
- CUDA/device identity;
- driver/runtime;
- topology fingerprint;
- environment snapshot;
- test results;
- raw machine JSON;
- relevant telemetry.

Live qualification must distinguish discovered facts from synthetic coverage.

## Layer 4 - Numerical/correctness differential tests

Purpose:
Ensure optimization/refactoring does not change workload semantics.

For Qwen:

- Reference remains correctness oracle;
- CUDA comparisons;
- token history/top-1 behavior;
- existing 0.10.0 nonregression gates.

For the second image architecture:

- establish an external/reference oracle;
- define reproducibility/tolerance appropriate to that workflow;
- compare semantic outputs at meaningful intermediate/final boundaries;
- do not claim pixel bit-identity unless actually required and observed.

## Layer 5 - Execution/performance experiments

Every retained optimization requires:

1. hypothesis;
2. exact baseline plan;
3. exact candidate plan;
4. same workload identity;
5. same relevant environment regime;
6. warmup methodology;
7. repeated samples;
8. correctness gate;
9. latency/throughput/memory/power evidence as relevant;
10. negative-result retention.

A benchmark-only path cannot qualify an optimization.

## Layer 6 - Server/API integration tests

Verify canonical surfaces:

- health/readiness;
- machine/topology/environment snapshots;
- workload/model identity;
- active plan;
- execution observations;
- generation/Decision;
- future image workflow;
- backpressure;
- cancellation;
- shutdown/restart;
- malformed requests;
- stale IDs/evidence.

The server remains a thin transport over AIR runtime authority.

## Layer 7 - GUI contract tests

The browser must be tested against canonical server data.

Verify:

- correct rendering from snapshots;
- event sequencing;
- reconnect;
- bounded history;
- explicit unknown/unavailable states;
- no browser-owned planner truth;
- no browser-owned topology reconstruction;
- controls use canonical commands;
- disconnect does not change execution;
- no unbounded DOM/event growth.

Reuse the existing AIR web doctor where possible.

Add browser automation only when it proves behavior that static/source/API
qualification cannot cover. Do not introduce a large web-tooling stack merely
for screenshots.

## Layer 8 - GUI visual/usability review

Automated correctness does not prove usability.

Review desktop and small-screen behavior for:

- information hierarchy;
- readable topology;
- readable timeline;
- distinction between semantic and physical graphs;
- distinction between fact, measurement, inference, and policy;
- bottleneck explanation;
- active/candidate plan comparison;
- missing-semantic explanation;
- error/recovery states;
- accessibility.

Screenshots are supporting evidence, not correctness authority.

## Layer 9 - User testing / dogfood phase

Prompt 11 is a deliberate user-testing and debugging phase.

### Session A - first launch

Task:
A person unfamiliar with current internals starts AIR and identifies whether
the machine is ready.

Observe:

- setup friction;
- unclear dependencies;
- hardware detection;
- terminology;
- error recovery.

### Session B - normal text inference

Task:
Load the qualified Qwen model, generate text, inspect execution.

User should be able to answer:

- which hardware is used;
- which plan is active;
- how much memory is used;
- where time went.

### Session C - "why is this slow?"

Task:
Use the Control Room to diagnose an intentionally or naturally slow run.

Success:
User finds evidence rather than guessing from raw utilization.

### Session D - image workflow

Task:
Load/run the second architecture.

Success:
GUI shows components, iteration, residency/offload, and output without
presenting it as token generation.

### Session E - unsupported/unknown workload

Task:
Load a readable package requiring unsupported semantics.

Success:
AIR does not crash or silently fall back. User sees exactly what is missing.

### Session F - memory pressure

Task:
Run near device-memory limits.

Success:
AIR reports capacity/plan decisions clearly and remains bounded.

### Session G - cancellation/restart

Task:
Cancel active work, restart AIR, reconnect browser.

Success:
Resources are reclaimed and the UI reconstructs canonical current state.

### Session H - novice versus expert modes

Novice:
Can operate defaults without understanding CUDA streams.

Expert:
Can inspect evidence, topology, plan, and timeline without losing raw detail.

## Defect workflow

Every material issue becomes:

```text
reproduction
 -> owning layer
 -> regression/characterization test
 -> root-cause fix
 -> normal verification
 -> targeted retest
 -> user-task retest when user-facing
```

Do not patch GUI symptoms when the server/runtime contract is wrong.

## Release-candidate freeze

Before Prompt 12:

- feature work stops;
- user-test release blockers are closed;
- known limitations are written;
- exact source identity freezes;
- qualification uses fresh isolated builds;
- GUI is tested against the exact candidate;
- machine discovery is re-run on the candidate;
- MEF compatibility is requalified;
- release artifacts/checksums are generated from the qualified source.

## AIR 0.11 release blockers

The release must not ship if any of these remain:

- machine discovery produces contradictory topology/environment state;
- adaptive plan changes semantics;
- stale evidence can select/promote a plan;
- GUI can become an execution/state authority;
- unsupported semantics silently fall back;
- second architecture requires a separate competing runtime inside AIR;
- cancellation/resource reclamation regresses;
- existing qualified Qwen path materially regresses without explicit evidence
  and release decision;
- a normal user cannot determine why AIR refused or selected an execution path.
