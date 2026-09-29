# AIR Adaptive Execution R0 - Initial Evidence Census

Status: WAVE 0 IN PROGRESS

## Existing evidence classes in AIR 0.10.0

### Release qualification evidence

AIR already retains strong evidence for:

- source identity;
- model identity;
- build/toolchain;
- CUDA qualification;
- Reference/CUDA comparison;
- public API behavior;
- backpressure;
- cancellation;
- reclamation;
- restart/shutdown;
- external MEF compatibility.

This discipline should be reused for adaptive execution rather than replaced.

### Request-level timing

Current request metrics expose high-level intervals:

```text
queue
plan preparation
plan eviction
prefill
TTFT
decode
total
```

Useful:
These provide user-visible latency decomposition.

Missing:
They do not identify every physical dependency or overlap.

### Planner evidence

Current planner decision/traces can report:

- objective;
- reason;
- eligible candidates;
- preparation state;
- transition estimate;
- break-even estimate;
- candidate traces.

Useful:
AIR already has a place for "why this plan" evidence.

Pressure:
Future estimates must identify the measurements/model used to produce them.

### Resource evidence

Current service snapshots expose:

- device bytes;
- KV bytes;
- prepared artifact bytes;
- reservation/capacity;
- page pool state;
- queue/active counts;
- batch counters.

Useful:
This is a strong base for residency and resource views.

Missing:
Generic component/buffer lifetime identity beyond transformer/KV concepts.

### Event evidence

Current events provide sequence order, wall-clock milliseconds, type, request,
and detail.

Useful:
Enough for coarse lifecycle/reconnect diagnostics.

Missing for execution compiler work:

- typed span identity;
- parent/dependency relationship;
- clock domain;
- device/stream;
- operation/physical-step identity;
- bytes transferred;
- measurement source;
- observation quality.

## Epistemic separation needed

Wave 0 should define explicit treatment for:

```text
FACT
MEASUREMENT
INFERENCE
POLICY
```

Examples:

Fact:
GPU compute capability.

Measurement:
H2D bandwidth observed during a defined probe.

Inference:
H2D transfer appears to dominate an idle interval.

Policy:
Prefer lowest latency under 12 GiB device memory.

These must not be serialized as equivalent "metrics."

## Measurement-methodology requirements

Every performance experiment should eventually bind to:

- source/program identity;
- model/resource identity;
- topology identity;
- dynamic environment identity;
- execution plan identity;
- objective;
- warmup procedure;
- sample count;
- measurement interval;
- profiler/instrumentation level;
- tool/runtime/driver identity;
- thermal/power context when material.

## Observer-effect rule

The act of observing execution can alter it.

At minimum AIR needs:

- normal bounded telemetry;
- detailed research tracing;
- profiling-overhead characterization;
- dropped-observation accounting.

Detailed tracing cannot be silently enabled during release-performance claims.

## Bottleneck evidence rule

A bottleneck label is an inference, not a raw measurement.

Future bottleneck output should retain:

- supporting raw evidence IDs;
- interval/operation affected;
- confidence/quality;
- plausible alternatives;
- current environment identity.

"Unknown" is an acceptable result.
