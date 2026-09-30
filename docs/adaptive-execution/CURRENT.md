# AIR Adaptive Execution Substrate R0 - Current

Updated: 2026-09-29
Status: PROMPT 5 CURRENT

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

## Prompt 3 qualified baseline

Prompt 3 is CLOSED / QUALIFIED.

Structural evidence:
`/home/emerson/Downloads/AIR-0.11-Prompt3-20260929-202022`

Overhead falsification:
`/home/emerson/Downloads/AIR-0.11-Prompt3-Overhead-20260929-204558`

Qualified facts:

- 13/13 CTests PASS;
- Prompt 2 nonregression PASS;
- Reference and CUDA off/normal/detailed timelines PASS;
- detailed CUDA backend spans: 49;
- transfer spans: 25;
- synchronization spans: 24;
- zero dropped spans;
- balanced normal overhead: +0.234% vs off;
- balanced detailed overhead: +0.545% vs off;
- detailed vs normal: +0.310%.

The earlier ~9-11% result is retained as materially confounded evidence.

Decision:

- normal observation is acceptable as the default for AIR 0.11;
- detailed remains an explicit diagnostic/research level;
- no observer optimization is justified by current evidence.

## Prompt 4 qualified baseline

Prompt 4 is CLOSED / QUALIFIED.

Qualified source:
`d7087bafa0bacdc6d302cfe3eaf4ba18ec7853e8`

WolfCat evidence:
`/home/emerson/Downloads/AIR-0.11-Prompt4-20260929-220128`

Qualification results:

- adaptive CPU preflight PASS, 13/13 CTests;
- fresh CUDA Release build PASS;
- CUDA 13/13 CTests PASS;
- real prepared-CUDA operation-site legality PASS;
- Reference/CUDA semantic-binding parity PASS;
- renamed-source CUDA execution/tactic independence PASS;
- real Qwen2.5 CUDA generation PASS;
- selected execution-plan implementations all legal;
- worktree remained clean;
- evidence checksums retained.

The Prompt 4 boundary is now qualified.

Core boundary:

```text
qualified semantic operation site
        ->
typed legal physical implementation set
```

Implemented operation sites:

- prefill transformer-block linear;
- decode transformer-block linear;
- decode output projection;
- prefill attention;
- decode attention.

`validate_execution_plan()` now delegates implementation legality to the
operation-site authority rather than directly knowing capability storage.

Architecture decisions:

- no universal operation catalog before the second architecture;
- no duplicate preparation/residency metadata registry;
- no standalone `/operations` endpoint before Prompt 5 ExecutionGraph.

Real prepared-CUDA capability coverage is in
`air-cuda-contract-tests`.

Final implementation/qualifier CPU preflight PASS:

`d70bc2e891351aeae6899c6d8227ae066f2dc405`

## Prompt 5 current

Prompt 5 is the ExecutionGraph R0 program.

Slice 5A census/architecture is COMPLETE.

Source inspection showed that a whole-request graph derived only from
`ExecutionPlan` would encode false precision because physical shape still
depends on live scheduler grouping, prefix/state hits, output mode, and
cancellation.

R0 therefore uses the smallest truthful seam:

```text
existing planner/scheduler/grouping decisions
        ->
physical invocation is concrete
        ->
derive immutable ExecutionGraph
        ->
existing PreparedModel / SequenceState execution
```

The graph is derived physical data. It does not become an executor, scheduler,
planner, state owner, semantic graph, or hardware authority.

Schema comparison and characterization requirements are recorded in:
`docs/adaptive-execution/PROMPT-05.md`

Slice 5B is CURRENT.

## Immediate next action

Implement only the additive 5B schema/projection slice:

- immutable ExecutionGraph R0 types;
- pure graph derivation from already-concrete physical invocation data;
- deterministic identity;
- serialization/inspection;
- characterization tests.

Do not dispatch graph nodes or change production execution behavior in 5B.

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
