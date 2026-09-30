# AIR Adaptive Execution Substrate R0 - Current

Updated: 2026-09-30
Status: PROMPT 7 CURRENT

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

Wave 6 / Prompt 7: image workflow oracle + package/component model.

Prompt 6 is CLOSED / QUALIFIED.

Prompt 7 is evidence-first. No broad image-runtime refactor is authorized
until one external image workflow is selected, frozen as an oracle, and its
component/state/iteration requirements are measured.

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

### Prompt 6A closed / falsified optimization hypothesis

Final source:

`ffa9f1e33d85ee276a8d189e9523b3a88bf84186`

Evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt6A-Prefill-20260930-033303`

Final result:

`PROMPT6A_PREFILL_BOUNDARY_CENSUS=PASS`

Measured long-prefill results:

- q32: TTFT `68544.017730 ms`, prefill `68541.987813 ms`,
  `18.225 tok/s`, 39 outputless waits;
- q64: TTFT `+0.493%` vs q32, throughput `-0.505%`, 19 waits;
- q128: TTFT `+5.103%` vs q32, throughput `-4.755%`, 9 waits.

Total outputless host-wait duration decreased at larger quantums while total
prefill/TTFT did not improve.

Decision:

- fewer outputless synchronization boundaries did not improve this workload;
- synchronization spans mostly reflect outstanding GPU work rather than
  synchronization-call overhead;
- do not remove/coarsen the synchronization boundary from this evidence;
- q32 remains the current default and the best tested quantum;
- no scheduler policy change is authorized from 6A.

The first 6A harness failure is retained separately as an endpoint-view
truncation measurement defect. It did not change AIR runtime behavior.

### Prompt 6B closed / qualified

Qualified source:

`395a2f73ffe58e1b491e384a6f3483d7fdd69f8f`

Evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt6B-Prefill-Tactics-20260930-050028`

Results:

- baseline: `18.000 tok/s`, `53389.560769 ms` prefill;
- reuse8: `77.099 tok/s`, `12465.888955 ms` prefill;
- dense-f32-cublas: `300.394 tok/s`, `3201.388061 ms` prefill;
- reuse8 throughput: `+328.330%` vs baseline;
- dense throughput: `+1568.875%` vs baseline;
- dense preparation: `34.480496 ms`;
- dense prepared artifact: `5,240,782,848 bytes` (~4.88 GiB);
- deterministic output equality PASS;
- `PROMPT6B_PREFILL_TACTIC_CENSUS=PASS`;
- qualifier exit code 0.

Decision:

- baseline is not competitive for this measured 1.5B long-prefill workload;
- reuse8 is a strong low-residency candidate;
- dense is a much stronger throughput candidate;
- dense preparation latency is small enough that residency/resource pressure,
  not cold preparation latency, is now the dominant unresolved tradeoff;
- no universal/default tactic is promoted yet.

### Prompt 6C closed / qualified

Retained exact-plan evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt6C-Transitions-20260930-100245`

Retained validation summary:

`/home/emerson/Downloads/AIR-0.11-Prompt6C-Transitions-20260930-100245/prompt6c-transition-summary-retained.json`

Final marker:

`PROMPT6C_RETAINED_EVIDENCE_VALIDATION=PASS`

Qualified exact-product-plan results:

- reuse8 prefill: `82.0553334475 tok/s`;
- dense prefill: `349.5992724 tok/s`;
- dense/reuse8 ratio: `4.26053076274046x`;
- dense optional prepared artifact: `4.880859375 GiB`;
- dense cold preparation: `44.3189706 ms`;
- dense eviction mean: `2.718012 ms`;
- hot second dense request prepared-state-hot: true;
- hot second dense request incremental preparation bytes: 0;
- dense re-preparation samples: `52.716044 ms`, `54.203769 ms`;
- preparation-only break-even: `4.751950219986411` prefill tokens;
- preparation+eviction round-trip break-even:
  `5.043379771405769` prefill tokens;
- low prepared-memory budget selected `reuse8-medium`;
- minimum-VRAM selected `reuse8-medium`;
- hidden dense residency after eviction: false.

Prompt 6 is CLOSED / QUALIFIED.

Its release-strategy exit criteria are satisfied:

- a real WolfCat bottleneck was reduced;
- correctness/nonregression passed;
- negative experiments were retained.

No 6D concurrency/fairness experiment is required for closure. Existing
scheduler contracts already cover round-robin fairness, decode-first budget,
admission/capacity, exact concurrency-region behavior, cancellation cleanup,
and transition-capacity accounting.

## Prompt 7 current

Prompt 7 is CURRENT / IMAGE WORKFLOW ORACLE + PACKAGE/COMPONENT MODEL.

Authority:

`docs/adaptive-execution/PROMPT-07.md`

Prompt 7 does not implement image generation inside AIR.

It first:

1. selects one concrete image workflow that runs externally on qualified
   hardware;
2. freezes exact workflow/runtime/resource identity;
3. captures external oracle output/behavior;
4. inventories components, residency/offload, iteration state, semantic values,
   and scheduler/solver behavior;
5. maps those requirements against current AIR;
6. derives the smallest Prompt 8 implementation packet.

No universal semantic IR and no image-specific second runtime/planner/scheduler
are authorized in Prompt 7.

## Immediate next action

Rerun the FLUX.2 Klein 4B Stage 7A selection preflight after the VAE source
identity correction.

The first run proved:

- diffusion artifact identity PASS;
- text-encoder artifact identity PASS;
- local VAE is not corrupt; it exactly matches the published
  `Comfy-Org/vae-text-encorder-for-flux-klein-4b` VAE.

The previous gate incorrectly expected the distinct
`Comfy-Org/flux2-dev` VAE used by the current stock ComfyUI template.

Run:

```text
bash scripts/qualify-adaptive-prompt7a-flux2-selection.sh \
  ~/Models/Media/Image/FLUX.2-Klein-4B \
  ~/Projects/AI-Runtimes/ComfyUI
```

Expected marker:

`PROMPT7A_FLUX2_SELECTION_PREFLIGHT=PASS`

A PASS freezes the exact local package and runtime identity.

Stage 7B must still execute the oracle and validate the local published
Klein-support VAE in practice before AIR core image semantics are derived.

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
