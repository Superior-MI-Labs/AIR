# AIR 0.11.0 Release Strategy - Adaptive Execution Foundation

Status: ACTIVE
Program: Adaptive Execution Substrate R0
Baseline: AIR 0.10.0 / v0.10.0
Candidate release: AIR 0.11.0

## Release claim target

AIR 0.11.0 should be able to truthfully claim:

> AIR can discover the local execution resources it directly owns, separate
> stable machine topology from dynamic environment state, observe where
> execution time/resources go, derive explicit physical execution plans from
> semantic computation plus machine evidence, expose those decisions through a
> human-visible Control Room, and demonstrate the architecture on Qwen2 plus one
> structurally different image-generation workflow.

AIR 0.11.0 must not claim universal model execution.

The architectural objective is stronger:

> unknown model/workflow structures should have a path to execution through
> explicit semantic contracts and implementations rather than requiring another
> model-family runtime.

## Standalone rule

AIR must remain useful with only AIR installed.

AIR may automatically discover:

- CPU and host-memory resources it can directly use;
- directly accessible accelerators and their AIR-supported capabilities;
- memory/storage relationships needed for AIR execution;
- AIR-compiled/available backend implementations;
- driver/runtime/toolchain identity needed to establish legality;
- dynamic resource availability and execution environment state.

AIR must not take over MEF's role by scanning/selecting unrelated external
provider runtimes such as llama.cpp, ComfyUI, ONNX Runtime services, or remote
model servers.

## Release success criteria

### Machine awareness

- one canonical read-only machine-discovery surface;
- deterministic topology identity for stable structural facts;
- dynamic environment snapshot separated from topology;
- CPU-only and CUDA-capable operation;
- synthetic coverage for laptop, desktop, unified-memory, multi-GPU, and
  datacenter-like layouts;
- partial/unknown observations remain explicit.

### Execution awareness

- typed execution spans/events;
- CPU preparation, transfer, device work, synchronization, and completion can
  be correlated where observable;
- observer overhead is bounded and measurable;
- raw evidence remains distinct from bottleneck inference.

### Physical planning

- one planner authority;
- explicit physical execution representation for the qualified Qwen path;
- implementation/tactic legality represented separately from semantic
  operation identity;
- at least one evidence-backed retained optimization that reduces a measured
  bottleneck without changing semantics.

### Second architecture

- one real image-generation workflow acts as the second architecture
  discriminator;
- its component/resource/loop/value requirements are documented from a working
  external oracle;
- AIR executes or lowers enough of the workflow to prove the common
  abstraction is not transformer-only;
- no parallel "diffusion runtime" is introduced.

### Extensibility

- an unknown semantic requirement produces a structured missing-semantic
  result;
- one test extension proves a new semantic implementation can be admitted
  without editing unrelated runtime/planner code;
- packages cannot execute arbitrary source/scripts merely because they were
  loaded.

### Human usability

AIR Control Room allows a user to see:

- detected machine;
- loaded workload architecture;
- selected plan;
- CPU/GPU/memory activity;
- execution timeline;
- bottleneck evidence;
- active versus candidate plan;
- unsupported semantics;
- qualification status.

The browser remains a thin surface.

### User testing

Before release qualification:

- first-time setup test;
- normal generation test;
- image-workflow test;
- "why is this slow?" diagnostic task;
- device/memory pressure task;
- unsupported-model task;
- restart/reconnect test;
- cancellation/failure recovery test;
- novice/default-mode usability;
- expert/research-mode observability.

Observed user problems become tracked defects or explicit release limitations.

## Prompt strategy

Each prompt is a bounded implementation/research package. A future agent should
be able to execute one prompt after refreshing the repository and reading the
active prompt record.

### Prompt 1 - Release contract + machine discovery foundation

Goals:

- freeze 0.11 release endpoint;
- finish machine-discovery census;
- introduce a canonical read-only host discovery API;
- expose one CLI machine-inspection surface;
- preserve scheduling behavior;
- test discovery on the development laptop.

Exit:

- AIR can answer "what CPU/RAM/CUDA resources do I directly see?" without
  loading a model;
- structural topology and dynamic availability are distinguishable in the new
  discovery contract;
- machine scan is safe and read-only;
- no external-provider discovery.

### Prompt 2 - Hardware topology / environment authority split

Goals:

- evolve HardwareTopology v1 without creating HardwareGraph #2;
- move volatile availability into ExecutionEnvironmentSnapshot;
- formalize observation provenance;
- integrate CUDA/CPU discovery;
- add server read-only machine/environment endpoints;
- qualify fixtures from CPU-only through multi-GPU.

Exit:

- topology identity does not change merely because free memory changes;
- stale environment snapshots are distinguishable;
- GUI can consume canonical machine state.

### Prompt 3 - Execution observation and timeline

Goals:

- typed execution spans;
- clock/correlation model;
- transfer/device/synchronization observations;
- bounded telemetry levels;
- observer-overhead benchmark.

Exit:

- one Qwen request can be reconstructed as a physical timeline;
- raw observations are retained separately from inferred bottlenecks.

### Prompt 4 - Semantic operation / implementation boundary

Goals:

- characterize Qwen operations;
- factor semantic operation identity from physical implementation/tactic;
- implementation legality contracts;
- no universal operation catalog.

Exit:

- AIR can ask which implementations are legal for a semantic operation without
  consulting source tensor naming.

### Prompt 5 - ExecutionGraph R0

Goals:

- explicit physical nodes/dependencies/placement/transfers/synchronization;
- project current Qwen execution into it;
- deterministic identity/replay;
- maintain one production path.

Exit:

- the graph describes existing real execution rather than a second executor.

### Prompt 6 - Schedule compiler + bottleneck optimization

Goals:

- use machine/environment/evidence to produce candidate physical schedules;
- test overlap, transfer staging, CUDA streams/events, graph capture, buffer
  reuse, and other evidence-supported candidates;
- retain only measured wins.

Exit:

- at least one real WolfCat bottleneck is reduced;
- correctness/nonregression passes;
- negative experiments retained.

### Prompt 7 - Image workflow oracle + package/component model

Goals:

- select one image-generation workflow that runs on qualified development
  hardware;
- establish external oracle output/behavior;
- census components, resource identities, latent values, iteration,
  scheduler/solver, residency/offload;
- derive missing AIR semantic requirements.

Exit:

- second-architecture requirements are evidence, not speculation.

### Prompt 8 - Second architecture through AIR

Goals:

- introduce only abstractions required by Qwen + image workflow;
- lower/execute meaningful image-workflow portions through AIR;
- preserve one runtime;
- avoid "diffusion engine" ownership.

Exit:

- common AIR abstractions are genuinely shared by both workload families.

### Prompt 9 - Unknown semantics / extension protocol

Goals:

- structured MissingSemantic result;
- operation/component version identity;
- trusted implementation registration;
- one test extension;
- security boundary against arbitrary package code.

Exit:

- a new semantic implementation can enter without editing unrelated AIR core.

### Prompt 10 - AIR Control Room GUI

Goals:

- Overview;
- Workload/Architecture;
- Machine;
- Execution timeline;
- Bottlenecks;
- Plan Lab;
- Memory/Residency;
- Evidence;
- Unsupported/Extensions;
- novice and research modes;
- reconnect/replay.

Exit:

- a human can explain what AIR is doing, where it is waiting, and why a plan
  was selected from canonical server state.

### Prompt 11 - User testing + debugging / RC hardening

Goals:

- dogfood on the development machine;
- scripted first-time-user sessions;
- usability observation;
- configuration-error testing;
- model/workflow failure testing;
- CPU-only/degraded-mode tests;
- memory pressure;
- thermal drift;
- cancellation/restart;
- GUI disconnect/reconnect;
- fix product defects at owning seams.

Exit:

- no known release-blocking usability/correctness defects;
- unresolved limitations documented;
- user-test evidence retained.

### Prompt 12 - Destructive qualification + AIR 0.11.0 release

Goals:

- full old-contract nonregression;
- machine-discovery qualification;
- execution-observation qualification;
- adaptive-plan destructive tests;
- second-architecture qualification;
- GUI qualification;
- frozen MEF provider compatibility;
- release provenance/checksums/tag/assets.

Exit:

- exact source qualified;
- release claims match evidence;
- AIR 0.11.0 published.

## User-testing method

User testing is not "click around until it feels okay."

Each session records:

- exact source/build;
- machine snapshot;
- user task;
- expected outcome;
- observed path;
- confusion/errors;
- runtime evidence;
- screenshots/logs when useful;
- defect classification;
- fix commit;
- regression test;
- retest result.

User-facing defects are categorized:

- correctness;
- discoverability;
- terminology;
- performance;
- configuration;
- recovery;
- visualization;
- unsupported behavior;
- documentation.

## Release decision rule

Do not ship merely because all planned features exist.

Ship only when:

- the product is explainable through the GUI;
- default behavior is safe;
- adaptive decisions have evidence;
- unsupported behavior fails clearly;
- normal user workflows survive user testing;
- old qualified behavior remains intact;
- source/evidence identities are frozen.
