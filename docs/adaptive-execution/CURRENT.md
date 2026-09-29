# AIR Adaptive Execution Substrate R0 - Current

Updated: 2026-09-29
Status: WAVE 0 CURRENT / PROMPT 1 LIVE QUALIFICATION PENDING

## Frozen baseline

AIR 0.10.0 is RELEASED / QUALIFIED.

Tag:
`v0.10.0`

Qualified source commit:
`3b728a1e45ae3c908aeb859b60cba3c2f5463506`

Release:
`AIR 0.10.0 — Modular Model Architecture Boundary`

AIR 0.10.0 remains the frozen evidence authority for that release.

## Active branch

`architecture/adaptive-execution-substrate-r0`

Created directly from the qualified 0.10.0 commit.

## Current mission

Establish the architecture and evidence program for AIR as a standalone
adaptive execution substrate where:

- computation is explicit data;
- hardware topology is explicit data;
- dynamic execution environment is explicit data;
- execution plans are derived data;
- performance observations are evidence;
- semantics remain authoritative and cannot be guessed.

## Current wave

Wave 0: freeze, assumption census, and observability contract.

No broad refactor is authorized yet.

## Work completed in Wave 0 initialization

Created and grounded against AIR 0.10.0 source:

- `BASELINE.md`;
- `ASSUMPTION-CENSUS.md`;
- `EVIDENCE-CENSUS.md`;
- `GUI-CENSUS.md`;
- `DATA-MODEL.md`;
- `GUI.md`;
- `QUESTION-BANK.md`;
- `AGENT-WORKFLOW.md`;
- `WAVE0.md`.

Important initial finding:

AIR already contains `HardwareTopology` v1. Do not create a parallel
HardwareGraph. Current topology mixes relatively stable physical facts with
dynamic available capacity and empirical link measurements, making
fact/state/measurement separation a primary Wave 0 question.

## AIR 0.11 release strategy

The active release strategy is:

`docs/adaptive-execution/RELEASE-0.11-STRATEGY.md`

Prompt 1 is:

`docs/adaptive-execution/PROMPT-01.md`

Testing/user-validation authority:

`docs/adaptive-execution/TESTING-STRATEGY.md`

## Prompt 1 implementation state

Implemented:

- host CPU/RAM discovery;
- topology fingerprinting that excludes dynamic availability and empirical
  link measurements;
- separate hardware environment availability snapshot;
- CUDA device augmentation;
- standalone machine-info CLI in human and JSON form;
- qualification script for CPU-only and CUDA builds.

No planner/scheduler adaptation is enabled by this work.

## Latest Prompt 1 evidence

The first live WolfCat qualification failed in the CPU-only build before
CTest.

Root causes were fixed:

- explicit Boost.JSON string construction in `air-cli machine-info --json`;
- missing CPU-only CUDA stub definitions for target-logprob executor methods.

The failure also validated the value of keeping CPU-only builds as a permanent
release gate: it exposed backend-contract drift that the CUDA path had hidden.

## Immediate next action

Rerun the Prompt 1 qualifier on WolfCat-Studio:

```text
bash scripts/qualify-adaptive-prompt1.sh
```

Prompt 1 must not close until the CPU-only and CUDA builds/tests pass and the
captured machine JSON matches the real development machine.

If qualification fails, repair the owning discovery/build/test layer before
starting Prompt 2.

## Current architectural hypothesis

The likely long-term lowering chain is:

```text
package/resource description
        ->
semantic computation
        ->
physical execution graph
        ->
kernel/device specialization
        ->
machine
```

This is a hypothesis, not yet a frozen IR.

## Second implementation

A multi-component image generation workflow is the intended second structural
discriminator.

Do not add a generalized semantic IR until the Qwen2 and second-implementation
requirements have been compared.

## Handoff rule

At the end of every substantial session:

- update this file with active wave and exact HEAD;
- record decisions in the relevant wave/design document;
- record failed experiments;
- leave one concrete next action;
- do not rely on chat context alone.
