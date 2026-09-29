# AIR Adaptive Execution R0 - Initial Assumption Census

Status: WAVE 0 IN PROGRESS
Basis: AIR 0.10.0 source + current adaptive-execution branch

This is the first pass. It records verified pressure points, not final refactor
decisions.

## Model / semantic assumptions

### Verified transformer-shaped state

`ModelConfig` currently carries:

- architecture identity;
- layer count;
- embedding size;
- FFN size;
- attention head count;
- KV head count;
- sliding-window field;
- RoPE dimensions/base/scaling;
- context length;
- vocabulary size;
- RMSNorm epsilon.

Conclusion:
`ModelConfig` is not a generic neural-workflow configuration object.

### Prepared semantics

`PreparedModelSemantics` currently binds:

- token embedding;
- output norm;
- output weight/bias;
- per-layer attention norm;
- Q/K/V weight/bias;
- attention output;
- FFN norm/gate/up/down.

Validation explicitly rejects any prepared architecture other than Qwen2.

Conclusion:
0.10.0 modularity separates source naming from execution but does not yet
generalize model topology.

### Request/serving assumptions

`InferenceRequest` is text/chat + generation configuration.

Serving:

- renders Qwen2 ChatML;
- tokenizes prompt/messages;
- checks token count against model context;
- schedules prompt tokens;
- generates token outputs.

Decision also encodes candidates into token sequences and scores them through
the same token-oriented model path.

Conclusion:
The current public production service is intentionally token-centric.

Do not force image/video workflows through `InferenceRequest`.

## Planning assumptions

`PlanningInput` currently consumes:

- one `ModelDefinition`;
- one token-oriented `RequestProfile`;
- one `RuntimeSnapshot`.

`ExecutionPlan` contains:

- backend kind;
- strategy identity;
- scheduler prefill quantum;
- KV page policy;
- quantized-linear tactics;
- attention tactics.

Conclusion:
The current planning authority is reusable, but its plan vocabulary is
transformer-specific.

Future work should evolve this one planner/execution-plan authority rather than
create a generic planner beside it.

## Scheduler assumptions

Verified current phases:

- prefill;
- decode.

Microbatch ordering gives decode phase priority and round-robin fairness within
each phase.

Capacity admission estimates sequence device bytes and prepared-plan artifact
bytes.

Conclusion:

- admission/fairness is a reusable runtime responsibility;
- prefill/decode, token budgets, and KV reservations are workload-specific
  expressions of that responsibility.

Wave 0 must identify a seam where generic admission remains one authority while
workload-specific execution phases remain explicit semantics.

## Hardware assumptions

AIR already defines `HardwareTopology` schema v1.

Node kinds:

- CPU;
- host memory;
- accelerator;
- storage;
- remote accelerator.

Link kinds:

- memory access;
- host-device;
- peer-device;
- storage-host;
- storage-device;
- remote.

Current node fields mix:

- structural identity/capability;
- total capacity;
- dynamic available capacity.

Current link fields mix:

- topology relationship;
- measured bandwidth;
- measured latency.

Conclusion:
The correct next step is likely a separation/evolution of the existing
hardware representation, not creation of a second HardwareGraph.

## Runtime snapshot assumptions

`RuntimeSnapshot` currently contains:

- free device memory;
- resident KV bytes;
- prepared artifact bytes;
- current strategy;
- device utilization.

Conclusion:
This is a small dynamic environment snapshot but lacks:

- topology identity;
- CPU state;
- transfer state;
- power/thermal/clock state;
- measurement provenance;
- competing-workload indicators.

## Evidence assumptions

`RequestMetrics` already records:

- queue time;
- plan preparation;
- plan eviction;
- prefill;
- TTFT;
- decode;
- total;
- request token counts;
- KV/prepared bytes;
- planner candidate traces.

`ServiceSnapshot` already exposes:

- planner state;
- scheduler state;
- capabilities;
- queue/active/rejection counts;
- request totals;
- batch counters;
- memory usage;
- sequence-state metrics;
- latency percentiles.

`RuntimeEvent` is currently:

- sequence;
- Unix milliseconds;
- type;
- request ID;
- free-form detail string.

Conclusion:
AIR has useful high-level evidence but not yet a typed physical execution
timeline.

The free-form event detail string is not sufficient for future causal
CPU/GPU/transfer analysis.

## GUI assumptions

The Web 3.2 API client consumes:

- /health;
- /model;
- /runtime;
- /events;
- /metrics;
- generation/chat/Decision endpoints.

Runtime view explicitly treats /runtime as structured authority and avoids
inventing a flattened parallel schema.

Diagnostics maintains a bounded browser-session merge of recent /events and
correctly labels it non-durable.

Metrics parses Prometheus text for presentation while stating /runtime is
preferred for structured state.

Conclusion:
The current browser already follows the thin-surface principle.

Pressure for the new control room is primarily server-side typed observability,
not browser-owned reconstruction.

## Initial refactor risk list

Highest-risk mistakes:

1. creating a second generalized model definition beside `ModelDefinition`;
2. creating a second planner beside `Planner`;
3. creating a second scheduler for image/diffusion workflows;
4. creating a second hardware graph instead of evolving `HardwareTopology`;
5. treating current measured bandwidth/available memory as stable topology;
6. making browser JavaScript infer bottlenecks from raw counters;
7. designing universal semantic operations before the second workload;
8. treating KV state as a universal state abstraction;
9. embedding CUDA-specific stream/kernel concepts into semantic computation;
10. preserving both old/new execution paths after refactor.

## Immediate Wave 0 source-inspection targets

Next source inspection should focus on:

- all construction/use sites of `HardwareTopology`;
- all producers of `RuntimeSnapshot`;
- planner implementations beyond `StaticPlanner`;
- CUDA stream/synchronization/copy behavior;
- model preparation allocation/residency;
- current qualification hardware fingerprints;
- runtime JSON/event serialization;
- browser refresh/poll cadence.
