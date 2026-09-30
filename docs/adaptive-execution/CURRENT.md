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

## Prompt 5 qualified baseline

Prompt 5 is CLOSED / QUALIFIED.

Final repaired handoff source:

`9363e85a2cfaea28fe79f592b6544d85e6b2b137`

5B evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt5B-20260929-225827`

5C correctness/evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt5C-20260930-010310`

5C balanced overhead:

`/home/emerson/Downloads/AIR-0.11-Prompt5C-Overhead-20260930-010442`

Qualified Prompt 5 facts:

- CPU/CUDA 13/13 CTests PASS;
- immutable ExecutionGraph R0 characterization PASS;
- current Qwen physical invocation seams are represented without a second
  executor;
- canonical topology-scoped placement is retained;
- real single CUDA graph/evidence observations are concordant;
- native CUDA batch shared-correlation gaps remain explicit rather than
  fabricated;
- normal/off modes remain graph-free;
- real detailed Qwen generation produced four graph observations and nine
  backend spans;
- `PROMPT5C_GRAPH_EVIDENCE=PASS`;
- balanced timing remeasurement PASS.

5C timing result:

- off: `1414.460855 ms`;
- normal: `1390.620811 ms`;
- detailed: `1410.377428 ms`;
- normal vs off: `-1.685%`;
- detailed vs off: `-0.289%`;
- detailed vs normal: `+1.421%`.

Interpretation remains conservative. Detailed graph observation showed a small
positive differential versus normal, but the same balanced run measured normal
faster than off and detailed slightly faster than off. Do not attribute the
full detailed-vs-normal delta to graph derivation from this experiment alone.

Prompt 5 does not make ExecutionGraph executable. The existing production path
remains authoritative.

## Prompt 6 current

Prompt 6 is CURRENT.

Prompt 6A is CURRENT / LIVE WOLFCAT CENSUS PENDING.

Source census is recorded in:

`docs/adaptive-execution/PROMPT-06.md`

The first falsification target is not a CUDA-code rewrite.

Current source shows that intermediate/outputless CUDA prefill chunks
synchronize before host KV transaction commit. Those waits are semantically
different from greedy/logit/target-logprob host-result waits and from
prepared-state transition synchronization.

The first experiment varies only the already-owned scheduler prefill quantum:

- 32 tokens;
- 64 tokens;
- 128 tokens.

It holds model, backend, tactics, token budget, prompt, temperature, prefix
cache, and observation mode constant.

The experiment measures:

- TTFT;
- prefill time/rate;
- outputless prefill synchronization count;
- outputless host-wait duration;
- greedy-result wait;
- H2D token enqueue count/bytes/duration;
- backend prefill-call count/duration;
- GPU telemetry.

Synchronization spans are treated as nested host waits that include outstanding
GPU work. They are not labeled as GPU-kernel or transfer duration.

Qualification/census authority:

`scripts/qualify-adaptive-prompt6a-prefill-boundaries.sh`

Initial census-script source:

`5d2cb7f20113e161151a22844e540a225ae42cac`

First WolfCat attempt from handoff
`db16e9a5876bfac142c5f959dfe0a38c7cad8356` passed CPU preflight and CUDA
server build, then falsified the harness before quantum comparison because the
script treated one bounded `GET /timeline` view as if it contained all six
long measured requests.

Evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt6A-Prefill-20260930-012327`

Root cause:

`InferenceService::execution_timeline()` returns the most recent 256 spans by
default. `dropped_spans == 0` describes ring-buffer eviction, not endpoint
view completeness.

The repaired harness captures/correlates timeline and graph evidence
immediately after every measured request and also proves observed prefill chunk
work units respect the requested quantum.

Repaired harness source:

`e9c13e4f5f097be1d7af3c50c0e60cd8e9bd93d0`

No AIR runtime behavior changed.

## Immediate next action

Pull the repaired branch and rerun on WolfCat-Studio:

```text
bash scripts/qualify-adaptive-prompt6a-prefill-boundaries.sh \
  ~/Models/AIR/Qwen2.5-1.5B-Instruct-Q4_K_M.gguf
```

No quantum is promoted from this single-request experiment alone.

If a candidate produces a repeatable improvement, the next slice must test
concurrency/fairness, TTFT distribution, cancellation, and native
multi-sequence prefill before any scheduling policy changes.

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
