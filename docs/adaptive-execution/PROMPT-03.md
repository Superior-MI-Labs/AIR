# AIR 0.11 Strategy - Prompt 3

Status: CLOSED / QUALIFIED
Title: Typed execution observation and physical timeline

## Prompt objective

Make AIR able to answer:

> What happened during this request, in what order, for how long, and at which
> execution layer was each observation made?

Prompt 3 does not optimize execution. It creates the evidence substrate required
before Prompt 6 schedule optimization can be trusted.

## Qualified baseline

Prompt 2 is CLOSED / QUALIFIED.

Qualified Prompt 2 source:
`abc74fda8bc02c9dd4e023bba422f5632ee44c9c`

The adaptive CPU preflight is also active and passing on the branch.

## Existing evidence to evolve

AIR already has:

- `std::chrono::steady_clock` request timing;
- queue, prefill, TTFT, decode, and total request metrics;
- bounded `RuntimeEvent` history;
- request/sequence IDs;
- explicit CUDA copy/synchronization sites;
- NVTX/profile ranges when enabled.

Current limitations:

- `RuntimeEvent::detail` is free-form text;
- request timing collapses repeated physical calls into totals;
- events use wall-clock milliseconds while request compute uses steady clock;
- CUDA transfer/synchronization steps are not correlated into a typed request
  timeline;
- browser consumers cannot reconstruct causal execution without inference.

## Internal slices

Prompt 3 is intentionally divided into four bounded slices.

### 3A - service-level typed timeline

Add a typed bounded execution-span timeline for already-observable service
phases.

Initial span scope:

- queue wait;
- prefill backend call;
- decode backend call;
- request total.

Batch calls may initially be represented as physical shared spans rather than
inventing per-request device timing.

Every span must identify its measurement scope honestly. Service-level spans are
not called CUDA kernel/device spans.

### 3B - backend observation bridge

Define the smallest sink/context required for backend implementations to emit
physical observations without giving the backend ownership of request history.

Requirements:

- optional/cheap when disabled;
- bounded ownership remains in the service/evidence layer;
- correlation ID propagation is explicit;
- Reference and CUDA contracts remain complete;
- disabled/stub parity is maintained.

### 3C - CUDA physical observations

Instrument only evidence-supported physical boundaries:

- host-device/device-host transfer;
- explicit synchronization;
- selected device-compute regions where timing can be measured without forcing
  additional synchronization.

Do not infer GPU duration from host call duration.

CUDA event timing may be introduced only with an observer-overhead study.

### 3D - endpoint and qualification

Expose typed timeline data through one canonical read-only endpoint.

Qualify:

- bounded history;
- monotonic ordering;
- request/sequence correlation;
- CPU/reference timeline;
- CUDA timeline;
- transfer/synchronization classifications;
- cancellation/failure spans;
- observer overhead;
- no inference/scheduler behavior change.

## Clock model

Prompt 3 must distinguish:

- monotonic duration/order authority: `steady_clock`;
- wall-clock presentation anchor: system Unix time.

Do not subtract unrelated clock domains.

A timeline snapshot should expose a wall-clock anchor plus monotonic offsets,
not pretend the clocks are identical.

## Candidate span contract

The first slice may use a structure equivalent to:

```text
ExecutionSpan
  schema_version
  observation_sequence
  request_id
  sequence_id
  scope
  category
  phase
  backend
  start_ns
  end_ns
  participant_count
  work_units
  success
```

Exact names may change during implementation.

Important semantics:

- `scope=service` means measured around an AIR service/backend call;
- later `scope=backend`/device observations are distinct;
- a host-call span around CUDA work is never relabeled as pure GPU compute;
- raw spans remain measurements, not bottleneck conclusions.

## Observation levels

Candidate levels:

- `off`: no typed span collection;
- `normal`: bounded service-level spans;
- `detailed`: backend/device observations when supported.

Default for development may be `normal`, subject to overhead evidence.

## Storage

Use bounded in-memory history owned by the existing service/evidence authority.

Do not add:

- a second profiler daemon;
- an unbounded trace store;
- a browser-owned execution history;
- an independent SQLite evidence authority.

## 3A tests

- a normal generation request emits queue, prefill/decode, and request spans;
- span observation sequence is monotonic;
- span time interval is non-negative;
- request and sequence IDs correlate;
- history is bounded;
- `off` level produces no spans;
- serialization is valid;
- existing `RequestMetrics` behavior remains unchanged.

## Pre-publish requirement

Before any Prompt 3 code is handed to WolfCat:

1. failure-retrospective checklist review;
2. backend/stub/link audit;
3. `scripts/preflight-adaptive.sh` PASS locally or in CI.

## Implementation state

Slices 3A through 3D are implemented for live qualification.

Implemented:

- bounded typed `ExecutionSpan` history owned by the existing service/evidence
  authority;
- `off`, `normal`, and `detailed` observation levels;
- service-scope queue, prefill, decode, batch, and request-total spans;
- explicit monotonic `steady_clock` offsets with a separate Unix-time
  presentation anchor;
- non-intrusive observation: allocation/recording failure increments
  `dropped_spans` instead of failing inference;
- explicit `ExecutionCorrelation` bound to each active `SequenceState`;
- correlation preserved through create, prefix restore, Decision branch
  restore, and adaptive same-backend restore;
- CUDA KV-cache correlation propagation;
- detailed CUDA host observations for evidence-supported transfer enqueue and
  explicit stream synchronization boundaries;
- CUDA checkpoint/fork state deliberately clears request correlation and
  requires rebinding when restored;
- read-only `GET /timeline`;
- server controls:
  `--execution-observation off|normal|detailed` and
  `--execution-span-capacity N`;
- CPU contract/serialization tests;
- adaptive CPU preflight PASS at
  `8a103810da93a074606c85738d6633b9b655e6d3`;
- `scripts/qualify-adaptive-prompt3.sh`.

Important scope truth:

The current CUDA backend observations measure **host-side CUDA API enqueue
duration and explicit synchronization wait duration**. They are not presented
as CUDA kernel/device execution duration.

Prompt 3 intentionally does not introduce CUDA-event kernel timing yet.
Doing so would require an explicit overhead study because extra event recording
or synchronization could perturb the execution being measured.

## Live qualification plan

The Prompt 3 qualifier:

1. runs the adaptive CPU preflight;
2. reruns Prompt 2 as nonregression;
3. proves a Reference service timeline;
4. proves CUDA `off` emits no spans;
5. proves CUDA `normal` emits service spans only;
6. proves CUDA `detailed` emits request-correlated backend transfer and
   synchronization spans;
7. requires zero dropped observations in the qualification runs;
8. measures observer overhead using mirrored mode order
   `off, normal, detailed, detailed, normal, off`;
9. uses 12 measured requests per mode after per-session warmup;
10. records the measured overhead without inventing a pass threshold before
    seeing WolfCat evidence.

The overhead result must be reviewed before Prompt 3 closes.

## First live WolfCat qualification

The first full Prompt 3 live qualification passed all structural/correctness
gates.

Evidence directory:

`/home/emerson/Downloads/AIR-0.11-Prompt3-20260929-202022`

Results:

- adaptive CPU preflight PASS;
- Prompt 2 nonregression PASS;
- current CTest matrix: 13/13 PASS;
- Reference normal timeline PASS;
- CUDA off timeline PASS;
- CUDA normal timeline PASS;
- CUDA detailed timeline PASS;
- request/sequence correlation PASS;
- dropped observations: zero in qualification;
- detailed CUDA backend spans: 49;
- detailed CUDA transfer spans: 25;
- detailed CUDA synchronization spans: 24.

Observed first-pass overhead:

```text
off median total:      1277.180235 ms
normal median total:   1399.643699 ms
normal vs off:         +9.5886%

detailed median total: 1416.761068 ms
detailed vs off:       +10.9288%
detailed vs normal:    approximately +1.2230%
```

Prompt 3 is **not closed** from this result.

Reason:

The apparent normal-mode cost is too large to accept as a default tracing cost,
but the evidence does not yet prove that the recorder itself causes the full
delta. The first measurement used mirrored order:

```text
off -> normal -> detailed -> detailed -> normal -> off
```

This reduces first-order time drift but still places both normal sessions in the
middle and does not make every mode occupy every ordinal position.

There is also an important falsification clue:

- normal recorded 26 spans and appeared about 9.59% slower than off;
- detailed added another 49 backend spans but was only about 1.22% slower than
  normal.

That pattern is inconsistent with a simple per-span cost explanation and
justifies a better controlled remeasurement before optimizing runtime code.

## Slice 3E - observer overhead falsification

Prompt 3 now includes a dedicated requalification step:

`scripts/requalify-adaptive-prompt3-overhead.sh`

The experiment uses a 3x3 balanced Latin order:

```text
round 1: off      normal   detailed
round 2: normal   detailed off
round 3: detailed off      normal
```

Therefore every mode occupies first, second, and third position exactly once.

Method:

- fresh qualified CUDA build from the exact branch head;
- 3 sessions per mode;
- 2 warmups per session;
- 6 measured requests per session;
- 18 measured requests per mode;
- primary comparison is median of per-session medians;
- NVIDIA temperature, P-state, SM/memory clocks, power, utilization, and memory
  are sampled around each live session when `nvidia-smi` is available;
- zero dropped spans remains required;
- off must emit zero spans;
- normal/detailed must emit typed spans.

The first draft of this harness was caught during pre-handoff review with a
heredoc/pipeline redirection defect. It was fixed before user handoff.

Exact script/preflight head:

`561bf34c1c3f66b85d2b4548684dfa5ca80e05b1`

Adaptive CPU preflight: PASS.

Decision rule after 3E:

- if the balanced result collapses the apparent ~10% delta, classify the first
  overhead result as materially confounded and retain the balanced evidence;
- if normal mode still shows a material repeatable cost, profile/optimize the
  recorder before making normal the default;
- detailed may remain research-only even if its overhead is higher, but its
  cost must be documented.

## Final Prompt 3E overhead falsification

Prompt 3E qualified on WolfCat-Studio.

Evidence directory:

`/home/emerson/Downloads/AIR-0.11-Prompt3-Overhead-20260929-204558`

Balanced 3x3 result:

```text
off median of session medians:      1486.142155 ms
normal median of session medians:   1489.623484 ms
detailed median of session medians: 1494.234779 ms

normal vs off:    +0.234%
detailed vs off:  +0.545%
detailed vs normal:+0.310%
```

Final gate:

```text
PROMPT3_OVERHEAD_FALSIFICATION=PASS
PROMPT3_OVERHEAD_REMEASURE=PASS
qualifier_rc=0
```

Interpretation:

The first apparent ~9-11% observation penalty is classified as materially
confounded by session-position/environment effects rather than accepted as
recorder cost.

The stronger balanced experiment makes each mode occupy each ordinal position
once and collapses the apparent cost to well below 1% on the qualified
development machine.

Release decision:

- `normal` observation is acceptable as the default for AIR 0.11;
- `detailed` remains appropriate for research/diagnostic use and is also low
  overhead in the qualified experiment;
- the first experiment is retained as negative evidence and as proof that
  performance experiments must actively control session order/environment;
- no recorder optimization is justified from current evidence.

Prompt 3 is CLOSED / QUALIFIED.

## Prompt 3 exit gate

Prompt 3 closes only when a real WolfCat request can be reconstructed as a
typed timeline containing honest service and CUDA physical observations, and
the overhead of observation is measured and acceptable.

No schedule optimization is authorized by Prompt 3.

## Next prompt

Prompt 4 separates semantic operation identity from physical implementation
identity using the evidence surfaces established here.
