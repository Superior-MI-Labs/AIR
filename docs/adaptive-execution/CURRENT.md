# AIR Adaptive Execution Substrate R0 - Current

Updated: 2026-09-29
Status: WAVE 0 CURRENT / PROMPT 3 OVERHEAD FALSIFICATION CURRENT

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

## Prompt 1 qualified baseline

Prompt 1 is CLOSED / QUALIFIED.

Qualified source:
`15cc24f3946707dbe2ee9843d9e8379c77b08e1e`

WolfCat evidence:
`/home/emerson/Downloads/AIR-0.11-Prompt1-20260929-175415`

Both CPU-only and CUDA builds passed 12/12 CTests. Canonical machine discovery
correctly identified the i7-11800H, 31.08 GiB RAM, and RTX 3080 Laptop GPU
(`sm86`) while keeping dynamic availability separate from structural
fingerprinting.

## Prompt 2 qualified baseline

Prompt 2 is CLOSED / QUALIFIED.

Qualified source:
`abc74fda8bc02c9dd4e023bba422f5632ee44c9c`

WolfCat evidence:
`/home/emerson/Downloads/AIR-0.11-Prompt2-20260929-184345`

Results:

- Prompt 1 nonregression PASS;
- CPU-only 12/12 CTests PASS;
- CUDA 12/12 CTests PASS;
- `GET /machine` PASS;
- `GET /environment` PASS;
- endpoint identity validation PASS;
- model load reduced observed GPU availability by 1,010,237,440 bytes while
  the structural topology fingerprint remained unchanged.

This closes the topology/environment authority split.

## Development-process change

Several qualification failures in Prompts 1-2 exposed preventable compile,
stub, harness, and build-graph mistakes.

These are now converted into repository-level prevention:

- `docs/adaptive-execution/FAILURE-RETROSPECTIVE.md`;
- mandatory interface/build-graph audit in `AGENT-WORKFLOW.md`;
- Layer 0 pre-publish gate in `TESTING-STRATEGY.md`;
- `scripts/preflight-adaptive.sh`;
- GitHub Actions CPU-only adaptive preflight.

Future prompt code must pass preflight before being handed off for live
WolfCat qualification.

## Immediate next action

Do not begin Prompt 3 implementation until the new CPU preflight itself has
passed on the branch.

After that, open Prompt 3: typed execution observation and physical timeline.

## Prompt 3 current

Prompt 3 authority:

`docs/adaptive-execution/PROMPT-03.md`

Implementation is complete through the live qualification harness.

Current implementation includes:

- typed bounded service execution spans;
- `off|normal|detailed` observation levels;
- explicit request/sequence correlation through backend sequence ownership;
- request correlation rebinding across prefix restore, Decision branches, and
  adaptive same-backend restore;
- CUDA detailed host observations for H2D/D2H enqueue operations and explicit
  stream waits;
- non-intrusive dropped-span accounting;
- read-only `GET /timeline`;
- configurable observation level/capacity;
- Prompt 3 live qualification with observer-overhead measurement.

Scope guardrail:

CUDA transfer observations currently describe host-side asynchronous enqueue
duration. Synchronization observations describe host wait duration. Neither is
called pure GPU kernel duration.

Pre-publish gate:

Adaptive CPU preflight PASS at
`8a103810da93a074606c85738d6633b9b655e6d3`.

## Immediate next action

Run on WolfCat-Studio:

```text
bash scripts/qualify-adaptive-prompt3.sh \
  ~/Models/AIR/Qwen2.5-1.5B-Instruct-Q4_K_M.gguf
```

Do not close Prompt 3 merely because the structural timeline checks pass.
Review `observer-overhead.json` and determine whether normal/detailed
observation overhead is acceptable before advancing to Prompt 4.

## Prompt 3 live result

Prompt 3 structural/correctness qualification is PASS.

Evidence:
`/home/emerson/Downloads/AIR-0.11-Prompt3-20260929-202022`

Observed:

- 13/13 CTests PASS;
- Prompt 2 nonregression PASS;
- Reference timeline PASS;
- CUDA off/normal/detailed timeline PASS;
- detailed CUDA backend spans: 49;
- transfer spans: 25;
- synchronization spans: 24;
- zero dropped spans.

The first observer-overhead experiment reported:

- normal vs off median: +9.5886%;
- detailed vs off median: +10.9288%;
- detailed vs normal: approximately +1.2230%.

Prompt 3 remains open because this is too large to accept for default
observation and the first experimental ordering does not sufficiently
disentangle recorder cost from session-position/thermal effects.

## Prompt 3E current

Run the dedicated overhead falsification:

```text
bash scripts/requalify-adaptive-prompt3-overhead.sh \
  ~/Models/AIR/Qwen2.5-1.5B-Instruct-Q4_K_M.gguf
```

The 3E experiment uses a 3x3 balanced Latin order so off, normal, and detailed
each occupy each ordinal session position once. It records GPU environment
telemetry and compares medians of per-session medians.

The exact 3E script passed adaptive CPU preflight at:

`561bf34c1c3f66b85d2b4548684dfa5ca80e05b1`

Do not begin Prompt 4 until this result is reviewed.

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
