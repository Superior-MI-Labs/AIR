# AIR Adaptive Execution - Agent Workflow

Purpose: keep long-running architecture work correct under limited AI context
windows and across multiple coding agents.

## Context-window rule

A coding agent should not load the whole repository or the whole program
history by default.

The normal context packet is:

1. repository `AGENTS.md`;
2. `docs/adaptive-execution/START-HERE.md`;
3. `docs/adaptive-execution/CURRENT.md`;
4. `docs/adaptive-execution/PROGRAM.md`;
5. the current wave document;
6. only source/tests directly involved in the task.

Use targeted retrieval for everything else.

## One wave at a time

A session should normally work on one wave and one ownership seam.

Do not mix:

- hardware schema refactor;
- semantic IR design;
- scheduler redesign;
- GUI rewrite;
- image integration;

in one implementation prompt.

Cross-wave implications belong in notes/question bank until their wave opens.

## Agent task packet

Every coding-agent prompt should contain:

- exact repository and branch;
- exact baseline/head if relevant;
- active wave;
- one objective;
- owning layer;
- invariants that may not change;
- files likely relevant;
- characterization/regression tests required;
- forbidden parallel architecture;
- verification commands;
- evidence to retain;
- stop condition.

A task without a stop condition is incomplete.

## Read less, verify more

Prefer:

```text
contract -> relevant source -> relevant tests -> machine evidence
```

over loading broad narrative history.

If a source file is large, retrieve the relevant symbols/ranges rather than
copying the whole file into context.

## Long evidence

Do not paste massive build logs into architecture documents.

Retain logs as artifacts/files and summarize:

- command;
- source identity;
- environment identity;
- exit status;
- failed gate;
- key measurements;
- evidence path/hash.

Future agents can retrieve the exact evidence only when needed.

## Refactor discipline

For any refactor:

1. state the current owner;
2. write/confirm characterization tests;
3. identify the new owner;
4. migrate one read/write path;
5. delete or disable the old path in the same wave when safe;
6. run regression/qualification gates;
7. search for remaining old ownership;
8. record the result.

No bridge may silently become permanent architecture.

## Root-cause rule

If a test/qualification gate fails:

- reproduce the exact failure;
- classify product vs harness vs environment;
- add a regression test when the failure represents product behavior;
- repair the owning layer;
- do not weaken thresholds merely to pass;
- retain negative evidence.

Every failure review must also answer:

1. what failed;
2. what owning-layer defect caused it;
3. why the previous review/test process missed it;
4. what practical permanent check can catch that class earlier.

If a practical prevention exists, implement it before advancing.

The accumulated failure-derived rules are maintained in
`docs/adaptive-execution/FAILURE-RETROSPECTIVE.md`.

## Mandatory pre-publish preflight

Before asking a user to pull/run new adaptive-execution code, the integration
agent must perform a dedicated pre-publish review.

Audit:

- before changing a pure virtual method, enumerate every derived override and
  test double; prefer additive wrappers when the new concern does not belong in
  every implementation;
- before adding an internal dependency to a test, inspect the target's existing
  CMake link boundary and choose the narrowest contract test that can prove the
  behavior;
- every changed public declaration and its definition owner;
- real backend and disabled/stub parity;
- each consumer target and its CMake link dependency;
- install/export implications of public headers;
- iterator/pointer/value type correctness;
- Result/Status propagation;
- overload/signature consistency;
- string/JSON compatibility conversions;
- lifetime-sensitive span/string_view usage;
- qualifier failure/cleanup behavior.

Then run, or obtain a passing CI result for:

```text
bash scripts/preflight-adaptive.sh
```

This CPU-only gate is mandatory even when the active work is CUDA-focused.

CUDA/device qualification remains a separate live-machine gate.

## Parallel-agent rule

Parallel agents may investigate independent questions.

Parallel agents should not concurrently modify:

- the same canonical schema;
- the same scheduler authority;
- the same planning contract;
- the same current-state file.

One integration agent owns reconciliation.

## Subagent partition examples

Good:

- Agent A inventories hardware assumptions.
- Agent B inventories timing/metrics surfaces.
- Agent C inventories model/transformer assumptions.
- Integration agent reconciles findings.

Bad:

- three agents each design their own HardwareGraph.

## Context checkpoint

Before context becomes large, update the owning wave record with:

- verified facts;
- decisions;
- rejected alternatives;
- exact current source identity;
- unresolved questions;
- next test/implementation action.

Then a fresh agent can continue from source + checkpoint rather than inherited
conversation memory.

## Commit discipline

Prefer small coherent commits with one architectural purpose.

Do not combine mechanical formatting with semantic refactors.

Commit messages should make ownership movement visible, for example:

- `test: characterize runtime snapshot timing`
- `refactor: separate dynamic memory state from hardware topology`
- `feat: add execution observation timeline schema`

## Generated-code / AI failure modes to watch

AI coding agents commonly:

- duplicate an existing abstraction under a new name;
- create convenience state that becomes a second source of truth;
- preserve old and new pipelines "temporarily" forever;
- invent generic types before a second implementation exists;
- make broad changes because context no longer includes original constraints;
- change tests to match implementation instead of preserving contracts;
- assume a current API from memory rather than inspecting source;
- conflate static hardware capability with dynamic measured performance;
- conflate semantic graph with physical execution graph;
- insert fallback behavior to make demos work;
- optimize benchmark-only paths;
- lose provenance between measurements and decisions;
- broaden public claims beyond qualification.

Every review should actively search for these failure modes.

## Agent-facing architecture rule

When an abstraction becomes too broad to understand inside one context window,
split documentation by responsibility, not by arbitrary file size.

Keep the small program spine stable. Put detail into focused records:

- hardware;
- evidence;
- semantic operations;
- execution graph;
- scheduler;
- GUI;
- second architecture;
- qualification.

## End-of-session handoff

A substantial session ends with:

```text
STATUS
ACTIVE WAVE
EXACT HEAD
WHAT CHANGED
WHAT WAS VERIFIED
NEGATIVE RESULTS
OPEN QUESTIONS
NEXT ACTION
DO NOT DO
```

Update `CURRENT.md` only with durable current state. Put detailed reasoning in
the owning design/wave record.
