# AIR Adaptive Execution Substrate R0 - Current

Updated: 2026-10-01
Status: PROMPT 8 CURRENT

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

Prompt 8: second architecture through AIR.

Prompts 1-7 are CLOSED / QUALIFIED.

Prompt 8 is using the qualified Qwen2 + FLUX.2 comparison to evolve only
structure that both workloads have falsified or required.

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

## Prompt 6 qualified record

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

## Prompt 7 closed / qualified

Prompt 7 - Image workflow oracle + package/component model - is CLOSED /
QUALIFIED.

Authority:

`docs/adaptive-execution/PROMPT-07.md`

Final retained Prompt 7G evidence:

`~/Downloads/AIR-0.11-Prompt7G-FLUX2-Residency-20261001-164618`

Final retained-evidence marker:

`PROMPT7G_RETAINED_EVIDENCE_REANALYSIS=PASS`

Qualified final observations include:

- semantic observer non-intrusion PASS;
- baseline and observed RGB pixel SHA-256 both
  `c3a4278c608408df5019cf15162707e29263dee76e7a0114a6b1dcf2c29e1aa6`;
- text encoder, FLUX.2 denoiser, and VAE all observed at load boundaries;
- 26 transition events;
- 20 residency timeline points;
- 324 approximately-100ms GPU telemetry samples;
- peak device memory `11791 MiB`;
- minimum free device memory `4194 MiB`;
- all three components classified `released-before-forced-free`;
- every required component absent both at the beginning and end of the explicit
  forced-free boundary;
- registry disappearance bracketed between observed sequences 50 and 53 over
  `1439.212009 ms`.

The `1439.212009 ms` value is a chronology bracket, not an unload-duration
measurement.

Exact asynchronous transfer completion and exact per-layer DynamicVRAM
residency remain explicitly unknown external-runtime details.

Prompt 7 therefore closes without deeper ComfyUI reverse engineering.

## Prompt 8 current

Prompt 8 - Second architecture through AIR - is CURRENT.

Authority:

`docs/adaptive-execution/PROMPT-08.md`

Prompt 8 generalizes only structure justified by the qualified Qwen2 + FLUX.2
comparison.

It does not introduce:

- a universal tensor IR;
- a universal semantic catalog;
- a second runtime;
- an image-specific scheduler/planner/residency authority;
- a Python workflow runtime.

### Stage 8A closed / qualified

Stage 8A introduces the workload request boundary without changing numerical
execution or planner/scheduler policy.

Published implementation:

- `include/air/workload.hpp`;
- `src/runtime/workload.cpp`;
- `tests/workload_contract_tests.cpp`;
- CMake target `air-workload-contract-tests`.

Current concepts:

- `ExecutionWorkloadKind::autoregressive_tokens`;
- `ExecutionWorkloadKind::iterative_state`;
- `WorkUnitKind::tokens`;
- `WorkUnitKind::iterations`;
- existing `RequestProfile` retained exactly as the autoregressive request
  contract;
- `IterativeRequestProfile` with finite iteration count and active instances;
- discriminated `ExecutionWorkloadProfile`.

No manifest, planner, scheduler, backend, numerical, or graph behavior has been
changed by this slice.

The first exact-head Stage 8A preflight failed at link time because Prompt 8A
initially reused the already-owned `air::WorkloadKind` name from the Decision
semantic contract. This was an ownership collision, not a numerical/runtime
failure.

The correction keeps the existing Decision axis
`generation | bounded_decision` unchanged and names the new physical
execution-structure axis `ExecutionWorkloadKind`. The discriminated physical
planning profile is now `ExecutionWorkloadProfile`.


Stage 8A qualified source:

`0768b749d466f8084add09088be993060f63a564`

Adaptive CPU preflight:

`36930036554` -> SUCCESS.

### Stage 8B current

Stage 8B is now addressing identified prepared-resource residency.

First falsified assumption:

`resident prepared bytes >= candidate prepared bytes`

is not a valid general hot-state test once more than one prepared resource can
exist. Equal bytes from the wrong resource must not satisfy a requirement.

Stage 8B slice 1 qualified source:

`5b544b0a7c39ef0923abbe9a95b59db180d6f5e1`

Adaptive CPU preflight `36930733784`: SUCCESS.

Stage 8B slice 2 is CURRENT.

`PreparedModel` now exposes identified prepared-resource residency from the
existing backend-owned counters. CUDA's dense-FP32 optional prepared state is
required to transition from nonresident -> resident -> nonresident under the
existing prepare/trim authority, while identity remains stable.

The aggregate compatibility byte view remains a projection of those same
executor counters and is not independently writable.

Slice-2 implementation source:

`604ebbd6bb35e2df1ef6b9845ba6d2b294cf2926`

Adaptive CPU preflight `36931183589`: SUCCESS.

Canonical WolfCat qualifier:

`scripts/qualify-adaptive-prompt8b-resources.sh`

WolfCat Stage 8B slice-2 evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt8B-Resources-20261001-184458`

Final marker:

`PROMPT8B_PREPARED_RESOURCE_RESIDENCY=PASS`

Results:

- CPU 15/15 PASS;
- CUDA 15/15 PASS;
- identified dense prepared-resource transition PASS;
- semantic parity / graph concordance / renamed-source independence PASS;
- real Qwen CUDA generation nonregression PASS.

Stage 8B slice 2 is CLOSED / QUALIFIED.

Stage 8B slice 3 is CURRENT.

The planner is being migrated from anonymous
`RuntimeSnapshot::prepared_artifact_bytes` to identified
`PreparedResourceResidency` records.

Hot state now requires resource identity. Eviction evidence is tied to an
actually resident incumbent resource that the candidate would drop.

Manifest prepared bytes remain aggregate measured capacity evidence in this
slice; AIR will not invent per-resource byte splits.

Slice-3 implementation source:

`97078266d410d4a33a36be1ecac8fd2b1cbdb66e`

Adaptive CPU preflight `36939282676`: SUCCESS.

Focused CUDA qualification now replays the retained Prompt 6C hot-dense and
dense -> reuse8 transitions through the identity-aware planner.

Canonical qualifier:

`scripts/qualify-adaptive-prompt8b-planner-resources.sh`

WolfCat Stage 8B slice-3 evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt8B-Planner-Resources-20261001-193414`

Final marker:

`PROMPT8B_PLANNER_RESOURCE_AUTHORITY=PASS`

Qualified:

- CUDA CTest 15/15 PASS;
- exact dense resource hot-state identity PASS;
- hot dense resident bytes `5240782848`;
- exact dense resource eviction identity PASS;
- eviction `2.401996 ms`;
- hidden dense residency after eviction: NO.

Stage 8B is CLOSED / QUALIFIED.

### Stage 8C current

Stage 8C makes work-unit interpretation explicit in physical invocation and
execution observation.

Implementation review found that the initial
`tokens | iterations` hypothesis is incomplete: existing CUDA transfer
observations already record byte counts in `work_units`.

Therefore the proven unit vocabulary is now:

- tokens;
- iterations;
- bytes.

No other unit is generalized without evidence.

Slice 1 requires current Qwen physical invocations to explicitly declare token
work, CUDA transfers to explicitly declare byte work, and non-zero untyped
measures to fail validation.

Graph-node-local heterogeneous counts are intentionally deferred to
ExecutionGraph R1 rather than falsely relabeled.

Immediate next action:

- exact-head Adaptive CPU preflight for Stage 8C slice 1;
- if green, run detailed CUDA observation/ExecutionGraph qualification and
  require token-vs-byte units to appear correctly without numerical or graph
  concordance regression.

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


### Stage 8C slice-1 qualification state

Initial slice-1 source:

`bb274cd7666452ab63d87b6c34990cafd81a616e`

Adaptive CPU preflight `36942384084` failed at CTest after a successful
build.

Classification: CHARACTERIZATION FIXTURE MIGRATION.

The new explicit-work-unit validator correctly rejected three legacy test
fixtures that directly constructed Qwen physical invocations without declaring
`tokens`.

Corrections preserve the new contract:

- native prefill batch fixture declares tokens;
- native decode batch fixture declares tokens;
- protocol graph fixture declares tokens;
- protocol observation fixture declares tokens;
- protocol JSON characterization requires explicit token unit;
- intentionally missing-unit graph test remains negative;
- work-measure validation is not marked noexcept because error Status
  construction may allocate.

Corrected source before this documentation update:

`ede0816fa9bae844d81cabc6b0d01d5439a78064`

Immediate next action:

- freeze branch writes;
- require exact-head Adaptive CPU preflight success;
- if green, package focused WolfCat detailed-observation qualification that
  proves service work is tokens, CUDA transfer work is bytes, zero-work
  synchronization may be unitless, and ExecutionGraph concordance remains
  unchanged.


### Stage 8C slice-1 live handoff

Corrected source:

`6984b768088a687548c2cdcf514ba38eee2d0260`

Adaptive CPU preflight `36942691588`: SUCCESS.

Canonical WolfCat qualifier:

`scripts/qualify-adaptive-prompt8c-work-units.sh`

The live gate proves the new units against real detailed CUDA observation:

- Qwen service work -> tokens;
- CUDA transfer work -> bytes;
- zero-work synchronization -> unitless;
- current Qwen ExecutionGraph invocation -> tokens;
- graph evidence remains concordant.

WolfCat Stage 8C evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt8C-WorkUnits-20261001-202558`

Final marker:

`PROMPT8C_TYPED_WORK_UNITS=PASS`

Qualified:

- CPU preflight 15/15 PASS;
- CUDA CTest 15/15 PASS;
- 5 non-zero service spans typed as tokens;
- 5 non-zero CUDA transfer spans typed as bytes;
- 4 synchronization spans zero-work and unitless;
- 4 Qwen graph observations typed as tokens;
- graph concordance PASS;
- topology fingerprint `hardware-topology:v1:f521e9bdd95ed645`;
- hardware resource `gpu0`.

Stage 8C slice 1 is CLOSED / QUALIFIED.

### Stage 8D current

Stage 8D introduces a discriminated workload-scoped physical invocation
boundary without changing the Qwen numerical/runtime path.

Chosen direction:

`AutoregressivePhysicalInvocation | IterativePhysicalInvocation`

The existing Qwen `PhysicalInvocation` remains the autoregressive member.
The iterative member carries only finite iteration count, active instances, and
physical placement. Its type fixes work-unit identity to iterations and has no
token output vocabulary.

ExecutionGraph R0 remains autoregressive-only until the later graph projection
stage. This slice does not create a second graph or executor.

Stage 8D slice-1 source:

`1f591f73b80ca56659a60cf4fd823a6d36ea7b8a`

Adaptive CPU preflight `36946782772`: SUCCESS.

Stage 8D slice 1 is CLOSED / QUALIFIED.

### Stage 8E current

Stage 8E now introduces the known FLUX.2 Klein semantic adapter contract.

The first slice is deliberately frozen to the qualified Prompt 7 oracle rather
than generalized to arbitrary diffusion requests.

It carries semantic identity/configuration only:

- prompt text / conditioning / seed / noise / schedule / latent / image;
- distinct positive/negative conditioning identities;
- distinct initial/sampled latent identities;
- 1024x1024, batch 1, 4 iterations;
- seed `432262096973490`;
- Euler, CFG 1;
- latent semantic geometry `[1,128,64,64]`;
- exact five-value sigma path;
- exact retained semantic-operation order.

It contains no storage dtype/layout or physical placement/policy.

Stage 8E slice-1 source:

`1c6c8d52110886948a5ea98f007609c3fa0af621`

Adaptive CPU preflight `36947237089`: SUCCESS.

Stage 8E slice 1 is CLOSED / QUALIFIED.

### Stage 8F current

Stage 8F now projects the frozen FLUX.2 semantic adapter into an ordered
component/resource physical plan under AIR's existing identified-resource
authority.

The three current component phases are:

- conditioning -> exact text-encoder artifact resource;
- iterative denoise -> exact denoiser artifact resource;
- decode image -> exact VAE artifact resource.

Only denoise carries four iteration work units. Placement is supplied, not
selected by the projection.

Prompt 7 staged-MB observations are not converted to AIR resident-byte
requirements. Expected component device bytes remain unknown.

Stage 8F slice-1 source:

`92b0dd2e183acd152e4603125ab3084f20ccaf25`

Adaptive CPU preflight `36947579369`: SUCCESS.

Stage 8F slice 1 is CLOSED / QUALIFIED.

### Stage 8G current

Stage 8G begins with a Qwen-preserving ExecutionGraph R1 data-model migration
before any FLUX.2 graph projection.

Slice 1 will:

- make graph invocation workload-discriminated;
- retain the existing Qwen invocation as the autoregressive member;
- make KV state explicitly autoregressive/optional;
- type true workload work at node level;
- stop using `work_units` as an untyped bucket for target/participant counts;
- allow compute regions to reference opaque prepared-resource identities;
- allow graph regions to reference opaque semantic value identities;
- project Qwen only;
- preserve production numerical execution.

The mandatory live gate is current Qwen detailed CUDA graph concordance.
FLUX.2 enters ExecutionGraph only after that gate survives.

Immediate next action:

- implement Stage 8G slice 1 behind characterization tests;
- exact-head CPU preflight;
- then focused WolfCat CUDA graph/evidence replay before any second-workload
  graph projection.


### Stage 8G slice 1 implementation

ExecutionGraph R1 Qwen-preserving migration is implemented.

The graph now has workload-discriminated invocation storage and optional
autoregressive state, while a temporary Qwen compatibility accessor keeps the
existing detailed observation path stable.

Node count semantics are corrected:

- true Qwen workload progress -> typed token `work_units`;
- target multiplicity -> `item_count`;
- greedy result multiplicity -> `item_count`;
- synchronization/unknown transfer work -> zero/unitless.

Qwen model compute regions can now reference the existing identified prepared
resource requirements. Generic semantic value references exist but remain empty
for Qwen.

Immediate next action:

- exact-head Adaptive CPU preflight;
- if green, create/run a focused Qwen CUDA graph R1 qualifier when a GPU runner
  is available;
- without a GPU runner, retain the CUDA gate as pending and continue only with
  graph-contract work that does not assert live device behavior.


### Stage 8G qualification state

Qwen-preserving R1 source:

`8b8e912a6d70167de7ca1a25f52c9158fc152836`

Hosted Adaptive CPU preflight: SUCCESS, 18/18 tests.

Real CUDA R1 concordance remains PENDING because the development GPU is
unavailable. This is recorded hardware evidence debt.

Stage 8G slice 2 now projects FLUX.2 into the same ExecutionGraph R1 authority.

The FLUX graph is explicitly `descriptive`, not AIR-executable. It contains
the three qualified component regions and exact resource/semantic identities,
but makes no transfer, synchronization, or implementation claim that AIR has
not measured/implemented.

Immediate next action:

- hosted CPU preflight for the shared Qwen + FLUX graph authority;
- then Stage 8H must decide and implement the smallest AIR-owned executable
  portion of FLUX.2 without depending on ComfyUI and without claiming
  unqualified device behavior.
