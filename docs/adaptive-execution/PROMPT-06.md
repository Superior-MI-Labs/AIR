# AIR 0.11 Strategy - Prompt 6

Status: 6A CURRENT / BOTTLENECK CENSUS
Title: Schedule compiler and bottleneck optimization

## Qualified baseline

Prompts 1-5 are CLOSED / QUALIFIED.

Prompt 5 established a deterministic physical-invocation ExecutionGraph R0 and
read-only planned-vs-observed evidence without creating a second executor.

Prompt 6 may change physical execution only through AIR's existing runtime,
scheduler, backend, and state authorities.

## Prompt objective

Use measured machine evidence to reduce a real physical bottleneck on
WolfCat-Studio without changing semantic behavior.

Every retained optimization requires:

1. a source-grounded mechanism;
2. an explicit performance objective;
3. a controlled baseline;
4. correctness/nonregression;
5. machine evidence;
6. preserved negative results;
7. one scheduler/runtime authority.

Do not choose an optimization because it is fashionable or theoretically fast.

## 6A source census

### Synchronization classes

Current CUDA synchronization sites fall into materially different classes.

#### Host-result waits

The host requires a result before the current API can continue:

- full-logit readback;
- greedy-token readback;
- target-logprob readback.

These waits are on real data dependencies. Their
`cudaStreamSynchronize()` duration includes outstanding GPU work on the stream
and must not be described as transfer latency alone.

For autoregressive greedy decode, the selected token is also an input to the
next decode step and to CPU-owned scheduling/state behavior. Removing the wait
would require a larger ownership change, not a local synchronization deletion.

#### Outputless prefill waits

Intermediate/outputless prefill chunks call
`cudaStreamSynchronize(outputless prefill)` before host KV state is committed.

No model output is copied to the host at this boundary.

The synchronization currently provides two properties:

- asynchronous CUDA failures become visible before host state commit;
- host KV transaction metadata is committed only after device work completes.

Therefore simply deleting this wait would violate the current transaction
contract.

However its frequency is directly affected by the already-owned scheduler
prefill quantum. Fewer, larger concrete prefill chunks may reduce the number of
such boundaries without changing transaction semantics.

#### Preparation/eviction waits

Dense-F32 and packed-DP4A preparation and tactic trimming synchronize before
prepared-state publication/destruction.

These are state-transition costs, not steady-state token-path waits. Existing
Strategy Lab transition evidence already owns their economics.

### Transfer classes

Current CUDA transfers include:

- prompt token IDs H2D for native prefill;
- target token IDs H2D for target-logprob reduction;
- greedy result D2H;
- full logits D2H;
- target-logprob values/flag D2H;
- KV page-table pointer H2D updates;
- KV copy-on-write D2D copies.

The token and greedy-result payloads are very small. Enqueue duration alone does
not establish a bandwidth bottleneck.

Full-logit readback is much larger, but temperature-zero generation already
uses device greedy selection and avoids that path.

### Allocation/residency observations

The main CUDA compute workspace is allocated once with the prepared executor.

KV pages are pooled but may grow on demand. Prepared dense/packed tactic state
has explicit preparation/eviction accounting.

No allocation optimization is authorized until measurements show a material
hot-path cost.

## 6A first falsification target - prefill scheduling boundaries

### Hypothesis

For a long single-sequence CUDA prefill, increasing
`prefill_quantum_tokens` within the already-qualified backend width may:

- reduce the count of outputless prefill synchronization boundaries;
- increase physical batch width;
- reduce host waiting/launch-boundary overhead;
- improve prefill throughput and/or TTFT.

It may also hurt multi-request responsiveness or fairness.

Therefore a single-request win is candidate evidence only, not promotion
evidence.

### Why this is the first experiment

This experiment:

- requires no production code change;
- uses the existing MicrobatchScheduler authority;
- uses the existing ExecutionPlan field;
- preserves the existing CUDA transaction boundary;
- can be measured with Prompt 3 timeline evidence;
- is directly represented by Prompt 5 physical invocation data;
- can be falsified cheaply before deeper stream/event refactors.

### Controlled variable

Compare:

- quantum 32;
- quantum 64;
- quantum 128.

Hold constant:

- exact AIR source;
- exact model;
- CUDA device;
- baseline physical tactics;
- token budget 256;
- prefix cache disabled;
- one active request;
- temperature zero;
- identical long prompt;
- one generated token;
- detailed observation.

### Evidence interpretation

For each quantum collect:

- response prompt token count;
- TTFT;
- prefill compute time;
- total request time;
- outputless-prefill synchronization count;
- outputless-prefill host-wait duration;
- greedy-result synchronization duration;
- H2D token enqueue count/bytes/duration;
- backend prefill call count/duration.

Synchronization durations are nested host waits, not additive GPU kernel
timings.

The experiment uses balanced session order to reduce order/thermal bias.

## 6A promotion rule

No quantum is promoted from this experiment alone.

If one candidate produces a repeatable single-request improvement, the next
slice must test at least:

- concurrent arrivals;
- queue/fairness behavior;
- TTFT distribution;
- throughput;
- cancellation;
- native multi-sequence prefill interaction.

If no meaningful improvement appears, retain the negative result and move to
the next bottleneck candidate.

## Forbidden in 6A

Do not:

- remove `cudaStreamSynchronize()`;
- add another CUDA stream;
- add CUDA Graph capture;
- add pinned-memory infrastructure;
- alter KV commit semantics;
- change the scheduler algorithm;
- make ExecutionGraph executable;
- promote a quantum based on source reasoning alone.

## 6A live evidence authority

`scripts/qualify-adaptive-prompt6a-prefill-boundaries.sh`

The script performs a fresh CUDA build and balanced 32/64/128 long-prefill
trials, retaining raw responses, timelines, GPU telemetry, summary JSON, and
checksums.

6A closes only after the WolfCat evidence is reviewed.


## 6A first live attempt - endpoint-view truncation falsified the harness

The first WolfCat 6A census attempt failed before comparing quantums.

Evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt6A-Prefill-20260930-012327`

Source:

`db16e9a5876bfac142c5f959dfe0a38c7cad8356`

Observed result:

- clean expected AIR head;
- adaptive CPU preflight PASS;
- CPU 13/13 CTests PASS;
- fresh CUDA server build PASS;
- census failed with:
  `RuntimeError: long prompt did not exercise outputless prefill synchronization`;
- final gate:
  `PROMPT6A_PREFILL_BOUNDARY_CENSUS=FAIL`;
- failed stage:
  `balanced-prefill-census`;
- exit code 1.

Root cause is the qualification harness, not a demonstrated absence of the
CUDA synchronization path.

The harness executed six long measured requests, then fetched `GET /timeline`
only once. The service observation ring had not dropped spans, but
`InferenceService::execution_timeline()` intentionally returns only the most
recent 256 spans by default.

Therefore:

`dropped_spans == 0`

means the service ring did not evict evidence. It does not mean one
`GET /timeline` response contains every span from all six long requests.

The harness then filtered that bounded endpoint view for each of the six
request IDs. Older measured requests could legitimately have zero matching
spans in the returned view, which the harness incorrectly interpreted as the
runtime failing to execute outputless prefill synchronization.

The CUDA source path remains:

`SequenceState::prefill_discard -> CudaExecutor::prefill_discard ->
prefill_impl(FinalOutput::discard) ->
synchronize_outputless_prefill -> commit_kv`.

Repair:

- fetch `/timeline` immediately after every measured request;
- correlate that newest bounded view to the just-completed request;
- fetch/correlate `/execution-graphs` per request as well;
- require current-request prefill backend-call evidence;
- require current-request outputless synchronization evidence;
- assert the maximum observed prefill backend-call work units do not exceed
  the configured 32/64/128 quantum;
- retain per-request raw endpoint snapshots.

Repaired harness source:

`e9c13e4f5f097be1d7af3c50c0e60cd8e9bd93d0`

No runtime, scheduler, CUDA, KV, or synchronization behavior changed.
