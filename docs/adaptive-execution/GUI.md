# AIR Adaptive Execution - Human Interface / Control Room

Status: DESIGN TARGET
The browser remains a thin surface over AIR authority.

## Human goal

A user should be able to answer, without reading source code:

1. What did I ask AIR to run?
2. What computation did AIR understand?
3. What hardware does AIR see?
4. What plan did AIR choose?
5. Why did AIR choose it?
6. What is running right now?
7. Where is time being spent?
8. What is waiting on what?
9. What memory/resources are resident?
10. Is AIR testing a better plan?
11. What evidence supports promotion?
12. What does AIR not understand?

## Product principle

The GUI should feel like a combination of:

- runtime control room;
- execution debugger;
- topology explorer;
- performance profiler;
- model/workflow inspector.

It must not feel like a wall of raw telemetry.

## Primary navigation

### 1. Overview

First screen answers:

```text
AIR status
active workload
current objective
selected execution plan
CPU/GPU activity
memory pressure
largest current bottleneck
qualification state
```

Use progressive disclosure. A normal user sees a concise health/readiness
surface; an expert can drill down.

### 2. Workload / Architecture

Visual graph of semantic computation.

For Qwen:

```text
tokens -> embedding -> transformer blocks -> logits -> sampling
```

For image workflow:

```text
prompt
 -> text encoder
 -> conditioning
 -> denoise loop
 -> VAE decode
 -> image
```

This view shows semantic components, not CUDA kernels.

Selecting a component reveals:

- semantic identity;
- input/output contracts;
- resources/weights;
- implementation candidates;
- unsupported/missing semantics.

### 3. Machine

Topology map:

```text
CPU ---- RAM ---- PCIe ---- GPU ---- VRAM
 |                           |
 caches                    copy/compute
```

For larger systems the same view expands into:

- NUMA domains;
- multiple GPUs;
- peer links;
- storage;
- remote accelerators when supported.

Each node/link has two visually separate classes:

- structural/capability facts;
- live measurements.

Never make measured bandwidth look like immutable hardware specification.

### 4. Execution

Live physical execution graph + timeline.

Suggested timeline lanes:

- request/admission;
- CPU preparation;
- host memory;
- H2D copy engine;
- GPU stream/queue 0;
- additional streams/queues;
- D2H;
- output/stream delivery.

The user can watch:

- queued;
- prepared;
- transferred;
- executing;
- synchronized;
- completed;
- cancelled.

Dependencies should be visible so idle time is interpretable.

### 5. Bottlenecks

Rank evidence, not opinions.

Example:

```text
Observed idle: 18.4 ms
Likely cause: host-to-device transfer
Evidence:
  GPU idle interval: 17.9 ms
  H2D copy overlaps interval: 17.5 ms
  CPU preparation already complete
Confidence: high
Candidate experiment:
  pinned double-buffer transfer
```

Show "unknown" when evidence is insufficient.

### 6. Plan Lab

Compare:

```text
ACTIVE PLAN
CANDIDATE PLAN
SHADOW RESULT
QUALIFICATION
```

For each plan:

- objective;
- placement;
- implementations;
- memory plan;
- transfer strategy;
- expected vs observed metrics;
- evidence sample count;
- thermal/environment conditions;
- reasons candidate is rejected/retained.

Promotion must require an explicit AIR policy/command, never a browser-only
state mutation.

### 7. Memory / Residency

Show:

- model/component residency;
- VRAM/RAM use;
- prepared artifacts;
- KV/state/latents;
- reusable/free pools;
- transfers/offloads;
- eviction candidates;
- lifetime timeline.

This becomes especially important for image/video workloads larger than VRAM.

### 8. Evidence

Inspect:

- raw observations;
- qualification manifests;
- experiment identities;
- baseline/candidate comparisons;
- provenance;
- stale evidence;
- negative results.

### 9. Unsupported / Extension

When AIR cannot execute a workload, the GUI should explain exactly why.

Example:

```text
Package recognized
Architecture recognized: example.foo
Unable to lower:
  semantic operation foo.temporal_mix@1 is unsupported

Known inputs:
  latent[B,T,C,H,W]
Known output:
  latent[B,T,C,H,W]

No legal implementation registered for this environment.
```

No generic "model unsupported" when more precise information exists.

## User simulations

### Scenario A: home user, one laptop GPU

User opens AIR.

Overview:

```text
Ready
CPU: Intel...
RAM: ...
GPU: RTX 3080 Laptop
VRAM free: ...
Objective: Balanced
```

They load Qwen2.

AIR shows:

```text
Package imported
Qwen2 semantics validated
Reference implementation available
CUDA implementation available
Selected: CUDA plan A
Reason: qualified + memory feasible
```

During generation the timeline visibly moves from tokenize -> prefill -> decode
and shows CPU/GPU activity and memory use.

### Scenario B: user asks "why is this slow?"

They click Bottlenecks.

AIR should not answer from a hard-coded heuristic.

It shows measured gaps and the strongest current inference.

The user can open a proposed experiment and see:

- what will change;
- what will remain semantically identical;
- risk/resource limits;
- expected measurement duration;
- current evidence strength.

### Scenario C: thermal throttling

AIR observes clocks/power/temperature changing.

The GUI distinguishes:

```text
hardware fact: RTX 3080 Laptop GPU
environment: thermal state changed
measurement: sustained throughput decreased
planner inference: prior plan evidence may be stale
```

If policy permits adaptation, a new candidate plan is evaluated.

### Scenario D: image generation

The architecture view shows components instead of pretending the workflow is
one transformer.

User can see text encoder, latent loop, denoiser, scheduler/solver, and VAE
with current residency.

If VRAM pressure triggers component offload, the timeline shows the transfer
and why it happened.

### Scenario E: enterprise multi-GPU

Default view groups resources rather than drawing hundreds of details.

```text
Host A
  CPU/NUMA 0
  GPU 0/1/2/3
Host B
  ...
```

Drill-down shows peer links and placement.

The GUI must not imply cluster-wide distributed execution until AIR actually
supports it. Remote resources can appear as observed/unsupported.

### Scenario F: unknown model

User loads a package AIR has never seen.

Desired UI:

```text
Format readable
Artifacts verified
Architecture/workflow identity: X
Lowering status: incomplete
Missing semantics: 2
No execution started
```

Then show the missing contracts and extension path.

## Configuration UX

Expose objectives as understandable presets:

- Balanced
- Lowest latency
- Highest throughput
- Lowest memory
- Power limited
- Deterministic/reproducible
- Advanced custom

Advanced users can inspect exact constraints.

Do not expose arbitrary internal fields merely because they exist.

## Live update architecture

The GUI should consume bounded typed snapshots/events.

Candidate transport:

- current state snapshot for reconnect;
- monotonically sequenced event stream;
- bounded timeline window;
- request/program/plan correlation IDs.

Reconnect must not require browser-owned history for correctness.

## Observer effect

The GUI/profiler can perturb the system being measured.

Requirements:

- measurement levels: off / normal / detailed research;
- known sampling intervals;
- bounded event queues;
- dropped-observation counters;
- profiling overhead measurements;
- heavy tracing disabled during performance qualification unless explicitly
  part of the methodology.

## Mobile/small-screen behavior

A phone should show:

- readiness;
- active workload;
- completion/progress;
- CPU/GPU/memory summary;
- primary bottleneck;
- stop/cancel;
- plan/objective identity.

Detailed topology and timelines can require desktop width.

## Accessibility

Do not encode state only by color.

Every visualization should have:

- textual state;
- readable labels;
- keyboard-accessible selection;
- table/raw-data alternative where feasible.

## Authority boundary

The GUI may request:

- start/stop/cancel;
- load/unload through canonical AIR APIs;
- objective changes;
- candidate experiment requests;
- plan promotion when permitted.

The GUI may not directly mutate:

- scheduler internals;
- resource ownership;
- model semantic bindings;
- execution graph memory;
- evidence records.

Every mutation goes through AIR's canonical command path and emits evidence.
