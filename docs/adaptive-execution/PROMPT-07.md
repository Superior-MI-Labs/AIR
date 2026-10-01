# AIR Adaptive Execution Substrate R0 - Prompt 7

Status: CURRENT
Wave: Image workflow oracle + package/component model

## Purpose

Prompt 7 introduces the first structurally different workload family.

It does not generalize AIR from theory.

It selects one real image-generation workflow that runs externally on the
qualified development machine, freezes that workflow as an oracle, inventories
its semantics/resources/state/control structure, and derives the smallest set
of missing AIR requirements forced by evidence.

Prompt 7 is a requirements/oracle wave.

Prompt 8 is the implementation/lowering wave.

## Authority and boundaries

AIR remains standalone.

Do not import:

- Builder system-structure authority;
- MEF provider-selection authority;
- IF continuity authority;
- ComfyUI workflow/runtime ownership as AIR runtime ownership.

An external image runtime may be used only as the behavioral/oracle authority
for Prompt 7 evidence.

AIR must not become:

- a ComfyUI wrapper;
- a diffusion-specific second runtime;
- a Python workflow executor;
- a package-supplied-code execution host.

## Frozen prior evidence

Prompt 6 is CLOSED / QUALIFIED.

The second architecture must preserve:

- one planner authority;
- one scheduler/admission authority;
- one hardware topology/environment authority;
- one PreparedModel/resource owner per loaded workload;
- one execution observation authority;
- one production runtime path.

Current token/Qwen-specific types are allowed to remain specific until the
second workflow proves a reusable replacement.

## Prompt 7 primary question

What does one real image-generation workflow require that current Qwen2 AIR
does not represent?

Answer from artifacts and execution evidence, not from a generic diffusion
taxonomy.

## Stage 7A - candidate workflow selection

Select one image-generation workflow that:

1. runs on the qualified WolfCat development machine;
2. fits the current device/host resource envelope with a documented execution
   policy;
3. has a reproducible external runtime/oracle;
4. can run deterministically enough to compare fixed-seed behavior;
5. exposes enough component/resource structure to study residency and offload;
6. is structurally different enough from autoregressive Qwen2 to pressure-test
   AIR abstractions;
7. does not require AIR to execute arbitrary package Python code.

Candidate selection must record:

- model/workflow identity;
- immutable artifact identities/checksums where available;
- runtime/framework identity;
- exact workflow graph/configuration;
- precision/quantization;
- image dimensions;
- scheduler/solver;
- step count;
- seed;
- device/offload policy;
- output identity/checksum or defined comparison method;
- peak VRAM/host RAM;
- wall-clock timing;
- machine/environment snapshot.

Do not select by popularity.

Select by usefulness as an architectural discriminator and reproducibility on
qualified hardware.

## Stage 7B - external oracle freeze

Before adding AIR image semantics, reproduce the selected workflow externally.

Required oracle evidence:

- exact source/runtime versions;
- exact model/component artifacts;
- deterministic seed/configuration;
- input prompt and other conditioning;
- output image artifact;
- output dimensions/format;
- component load/unload sequence when observable;
- total runtime;
- per-stage timings when observable;
- peak/representative VRAM;
- host RAM;
- offload/residency behavior;
- failure behavior under insufficient resources where practical.

A fixed seed does not imply bit-identical behavior across arbitrary
toolchains/hardware. The oracle contract must state the actual comparison
semantics supported by evidence.

## Stage 7C - package/component census

Inventory independently meaningful resources/components.

Candidate examples that must be proven or rejected by the selected workflow:

- text encoder(s);
- tokenizer;
- denoiser / transformer / UNet-like network;
- VAE/autoencoder encode/decode;
- latent initialization;
- scheduler/solver state;
- timestep/sigma schedule;
- conditioning tensors;
- guidance state;
- latent state across iterations;
- final image decode;
- optional preprocess/postprocess components.

For every component/resource, record:

- semantic role;
- immutable resource identity;
- input/output value meaning;
- dtype/shape;
- load/preparation requirements;
- residency lifetime;
- host/device placement observed externally;
- whether state persists across denoising iterations;
- whether the component can be independently loaded/offloaded;
- whether execution order is semantic or only physical.

Do not map these directly onto Qwen layers/KV concepts.

## Stage 7D - iteration and state semantics

Characterize the iterative region explicitly.

Questions to answer with the selected workflow:

- what value is carried from step to step?
- what schedule/timestep/sigma state changes each iteration?
- which model/resource calls repeat?
- which conditioning values are invariant?
- which state is stochastic versus deterministic from seed?
- where is random-number generation semantically observable?
- what may legally be recomputed?
- what must remain resident?
- what can be offloaded between phases?
- what termination condition defines completion?

Do not assume AIR needs a universal loop IR.

First describe the selected workflow's required iterative contract.

## Stage 7E - semantic value census

Determine which non-token values require explicit semantic identities.

At minimum investigate whether the selected workflow forces:

- image;
- latent;
- conditioning/embedding;
- timestep/sigma;
- random seed / RNG state;
- scheduler/solver state.

For each proposed semantic value, separate:

- meaning;
- storage dtype;
- shape;
- physical placement;
- mutability/state lifetime.

Tensor shape alone is not semantic identity.

## Stage 7F - operation/component boundary

Classify selected-workflow computation into:

1. semantic operations AIR may need to know;
2. component-level calls that may lower into multiple operations;
3. physical implementation choices that must not become semantic truth.

Examples are hypotheses only until proven by the workflow.

Do not freeze a universal operation catalog in Prompt 7.

## Stage 7G - resource/residency pressure

Use the selected workflow to pressure-test Prompt 6 lessons.

Measure or observe:

- independently resident component bytes;
- component transition/load cost;
- offload/reload cost where used;
- peak transient buffers;
- persistent iteration state;
- memory ceiling behavior;
- whether keeping a component resident amortizes its transition cost;
- whether execution phases create mutually exclusive residency opportunities.

The goal is to derive requirements for AIR's existing planning/resource
authorities.

Do not create an image-specific residency manager.

## Stage 7H - current-AIR gap map

Produce a gap table:

```text
workflow requirement
    current AIR representation
    status: reusable / transformer-specific / missing
    evidence
    proposed smallest evolution
```

Expected pressure areas include, but are not limited to:

- token-only RequestProfile;
- transformer-specific ExecutionPlan tactic vocabulary;
- KV-specific resource accounting;
- token-oriented InferenceRequest/public serving;
- semantic values beyond token sequences;
- iterative state/control;
- independently resident components;
- workload-specific objective/first-output semantics.

Do not change these types in Prompt 7 merely because they look narrow.

Prompt 8 owns implementation after the evidence map is complete.

## Evidence classes

Every Prompt 7 statement must be classified where material as:

- Fact;
- Measurement;
- Inference;
- Policy.

Examples:

Fact:
a component artifact SHA-256.

Measurement:
peak VRAM during the external oracle run.

Inference:
component offload appears necessary under the current memory ceiling.

Policy:
AIR should prefer a lower-residency plan.

Do not serialize inference as hardware fact.

## Required Prompt 7 artifacts

Create/maintain:

- `docs/adaptive-execution/PROMPT-07.md`;
- selected-workflow identity record;
- external-oracle evidence bundle;
- package/component census;
- iteration/state census;
- semantic-value census;
- current-AIR gap map;
- retained failed candidate/oracle attempts.

Machine evidence remains under `~/Downloads` during qualification and should
be referenced by exact directory.

## Required tests / gates

Prompt 7 does not require AIR to generate an image yet.

It requires:

1. current AIR CPU preflight remains green;
2. current Qwen qualification contracts are not modified to fit image
   concepts prematurely;
3. selected external image workflow completes reproducibly;
4. fixed workflow identity/configuration is retained;
5. component/resource/state census is complete enough to derive Prompt 8 work;
6. unsupported/unknown observations remain explicit;
7. no second AIR runtime/planner/scheduler/residency authority is introduced.

## Stop conditions

Stop Prompt 7 and retain negative evidence if:

- the selected workflow cannot run reproducibly on qualified hardware;
- its artifacts/runtime cannot be frozen well enough to form an oracle;
- resource pressure makes the workflow unsuitable as the development oracle;
- external execution is too opaque to support the required census.

In that case select another candidate by documented criteria.

Do not force an unsuitable workflow through AIR.

## Prompt 7 exit

Prompt 7 closes when:

- one concrete image workflow is externally qualified as the oracle;
- its package/components/resources/state/iteration are evidence-backed;
- current AIR gaps are explicitly mapped;
- the smallest Prompt 8 architectural requirements are derived;
- no universal semantic IR has been frozen from one image workflow;
- no image-specific second runtime authority has been introduced.

The output of Prompt 7 is evidence and a constrained implementation packet for
Prompt 8, not image execution inside AIR.

## First action

Perform Stage 7A candidate selection and external-oracle preflight.

Do not modify AIR core execution types before the selected workflow and oracle
are frozen.


## Stage 7A census implementation

Authority:

`scripts/census-adaptive-prompt7-image-candidates.sh`

Default inputs:

- image package root: `~/Models/Media/Image`;
- external oracle runtime root:
  `~/Projects/AI-Runtimes/ComfyUI`.

The census is read-only.

It records:

- AIR source identity;
- machine/RAM/storage/GPU snapshot;
- image candidate directory names and sizes;
- bounded package/config/component metadata;
- large artifact inventory without hashing every model weight yet;
- safe config excerpts;
- ComfyUI Git/runtime/Python/Torch/CUDA identity;
- available relevant Python packages;
- custom node names;
- ComfyUI model-search-path configuration;
- candidate-selection checklist;
- evidence checksums.

It intentionally does not:

- select a candidate by directory name;
- download/modify models;
- modify ComfyUI;
- start an image generation;
- compute full checksums for every multi-gigabyte candidate artifact;
- modify AIR core types.

Full artifact checksums are deferred until Stage 7A narrows to one candidate.

Expected marker:

`PROMPT7A_IMAGE_CANDIDATE_CENSUS=PASS`


## Stage 7A provisional selection

Provisional oracle candidate:

`FLUX.2 Klein 4B` distilled FP8 text-to-image workflow.

Local candidate components:

- diffusion:
  `~/Models/Media/Image/FLUX.2-Klein-4B/flux-2-klein-4b-fp8.safetensors`;
- text encoder:
  `~/Models/Media/Image/FLUX.2-Klein-4B/split_files/text_encoders/qwen_3_4b_fp4_flux2.safetensors`;
- VAE:
  `~/Models/Media/Image/FLUX.2-Klein-4B/split_files/vae/flux2-vae.safetensors`.

Selection rationale:

1. official Black Forest Labs reference implementation exists;
2. native ComfyUI Flux2 support is present in the pinned local ComfyUI commit;
3. official/native workflow structure exposes:
   - text conditioning;
   - explicit latent value;
   - explicit scheduler/sigma sequence;
   - stochastic noise/seed;
   - repeated sampler region;
   - separate VAE decode;
4. the local package is materially smaller than Qwen-Image-2.1 and is a lower
   risk first oracle on the 16 GiB WolfCat GPU;
5. it remains structurally different enough from autoregressive Qwen2 to force
   image/latent/iteration/resource questions;
6. the locally installed FP4 Qwen3-4B text encoder is a Comfy-Org published
   artifact, not an unknown user conversion.

Deferred candidate:

`Qwen-Image-2.1`.

Reason for deferral:

- retain as a later, higher-pressure residency/offload discriminator;
- its installed components total roughly 17 GiB on disk, with an ~8.7 GiB text
  encoder and ~6.8 GiB diffusion model, making it a more complex first oracle;
- Prompt 7 selection optimizes for reproducibility and architectural
  discrimination, not maximum memory pressure.

This is provisional until local artifact identity and exact pinned-runtime
support pass the Stage 7A selection preflight.

### Stage 7A FLUX.2 selection preflight

Authority:

`scripts/qualify-adaptive-prompt7a-flux2-selection.sh`

The gate is read-only.

It:

- verifies SHA-256 identity for the local diffusion/text-encoder/VAE artifacts;
- inspects safetensors headers/tensor structure without loading weights;
- freezes the pinned ComfyUI Git identity;
- requires native `EmptyFlux2LatentImage` and `Flux2Scheduler` support;
- verifies ComfyUI imports from the actual ComfyUI working directory;
- reports whether each selected component is already visible through standard
  local ComfyUI model directories;
- records machine state and checksums the evidence bundle.

Expected published artifact identities:

- diffusion FP8:
  `97ed34fe0567e436200f2faee3939b88f2b5d99f8af2a4dc16532c4245c0ccb6`;
- Qwen3-4B FP4 Flux2 text encoder:
  `3eab03a77adb0ee5304a4e677d5c10ac22f9049c1d7c894adca4f8bb39206ca8`;
- selected local Klein-support Flux2 VAE:
  `868fe7b343cc8f3a19dbcfcafbc3d5f888802be3f89bd81b65b3621a066ce8f3`;
- current stock-template `Comfy-Org/flux2-dev` VAE is retained as a
  documented alternative:
  `d64f3a68e1cc4f9f4e29b6e0da38a0204fe9a49f2d4053f0ec1fa1ca02f9c4b5`.

Expected marker:

`PROMPT7A_FLUX2_SELECTION_PREFLIGHT=PASS`

A PASS freezes the candidate identity but does not yet qualify the external
image oracle. Stage 7B still requires a fixed workflow/seed/configuration and
retained output/resource evidence.


## Stage 7A first selection-preflight failure

WolfCat evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt7A-FLUX2-Selection-20260930-141104`

AIR source:

`fe0215da2970a699d944cda960c93747fd16387c`

Observed:

- FLUX.2 Klein 4B FP8 diffusion SHA-256 matched;
- Qwen3-4B FP4 Flux2 text encoder SHA-256 matched;
- local `flux2-vae.safetensors` SHA-256 was
  `868fe7b343cc8f3a19dbcfcafbc3d5f888802be3f89bd81b65b3621a066ce8f3`;
- gate expected
  `d64f3a68e1cc4f9f4e29b6e0da38a0204fe9a49f2d4053f0ec1fa1ca02f9c4b5`;
- gate stopped before safetensors census / ComfyUI support / visibility checks.

Classification:

SOURCE-IDENTITY ASSUMPTION FAILURE, not corrupt local model evidence.

Public source verification established two different published
`flux2-vae.safetensors` artifacts:

1. `Comfy-Org/flux2-dev`
   - SHA-256:
     `d64f3a68e1cc4f9f4e29b6e0da38a0204fe9a49f2d4053f0ec1fa1ca02f9c4b5`;
   - current stock ComfyUI FLUX.2 Klein text-to-image workflow links this VAE.

2. `Comfy-Org/vae-text-encorder-for-flux-klein-4b`
   - SHA-256:
     `868fe7b343cc8f3a19dbcfcafbc3d5f888802be3f89bd81b65b3621a066ce8f3`;
   - repository metadata explicitly identifies
     `black-forest-labs/FLUX.2-klein-4B` as its base model.

The local WolfCat VAE exactly matches #2.

Corrective policy:

- do not overwrite or redownload the local VAE merely to match the stock
  workflow template;
- freeze the local published Klein-support VAE as part of the provisional
  oracle package;
- retain the stock-template VAE identity as an explicit workflow difference;
- require Stage 7B external oracle execution to validate that exact local VAE
  in the selected workflow;
- if Stage 7B fails specifically at VAE compatibility, retain the failure and
  then test the stock-template VAE as a controlled alternative.

The Stage 7A selection gate now expects the local published Klein-support VAE
identity and records both VAE source identities.


## Stage 7A final qualification

Status: CLOSED / QUALIFIED.

WolfCat evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt7A-FLUX2-Selection-20260930-143502`

Final marker:

`PROMPT7A_FLUX2_SELECTION_PREFLIGHT=PASS`

Qualified facts:

- FLUX.2 Klein 4B FP8 diffusion identity:
  `97ed34fe0567e436200f2faee3939b88f2b5d99f8af2a4dc16532c4245c0ccb6`;
- Qwen3-4B FP4 Flux2 text encoder identity:
  `3eab03a77adb0ee5304a4e677d5c10ac22f9049c1d7c894adca4f8bb39206ca8`;
- selected published Klein-support VAE identity:
  `868fe7b343cc8f3a19dbcfcafbc3d5f888802be3f89bd81b65b3621a066ce8f3`;
- safetensors component census PASS;
- pinned ComfyUI:
  `986c4d154ef8c288382ac87d956b52a2b640c8b3`;
- native `EmptyFlux2LatentImage` and `Flux2Scheduler` support present;
- ComfyUI source-tree imports PASS;
- Torch `2.11.0+cu130`, CUDA `13.0`;
- RTX 3080 Laptop GPU visible with 16 GiB class device memory;
- selected diffusion/text-encoder/VAE files are all visible to ComfyUI.

The selected oracle package is now frozen.

Prompt 7B is CURRENT.

## Stage 7B external oracle implementation

Authorities:

- `scripts/prompt7b_flux2_oracle.py`;
- `scripts/qualify-adaptive-prompt7b-flux2-oracle.sh`.

The oracle mirrors the native ComfyUI distilled FLUX.2 Klein structure:

```text
frozen prompt
    -> Qwen3-4B Flux2 text encode
    -> positive conditioning
    -> zeroed negative conditioning

fixed seed -> RandomNoise

1024x1024
    -> EmptyFlux2LatentImage (128 latent channels)

4 steps + 1024x1024
    -> Flux2Scheduler
Euler sampler
CFG=1

model + conditioning + noise + sigmas + latent
    -> SamplerCustomAdvanced
    -> final latent
    -> selected local Flux2 VAE
    -> 1024x1024 RGB image
```

Frozen oracle parameters:

- resolution: 1024 x 1024;
- steps: 4;
- sampler: Euler;
- CFG: 1;
- seed: `432262096973490`;
- batch: 1;
- custom nodes: disabled;
- ComfyUI result cache: disabled;
- PyTorch deterministic flag: enabled.

The driver executes the identical graph twice in one dedicated pinned ComfyUI
process.

The comparison contract is:

`bit-identical RGB pixels across two forced re-executions on the same pinned runtime/hardware`

The file-level PNG hash is also retained but pixel identity is the required
same-machine oracle reproducibility gate because container metadata need not be
the semantic image identity.

The wrapper additionally requires:

- clean AIR and ComfyUI worktrees;
- clean GPU compute baseline;
- dedicated localhost port;
- ComfyUI-visible model paths resolve to the same underlying canonical model
  files rather than independent duplicate copies;
- qualified Stage 7A evidence is provided;
- GPU/RAM telemetry is retained during execution;
- server resource/load/offload events are retained;
- dedicated oracle process is terminated and no compute process remains.

Expected marker:

`PROMPT7B_FLUX2_EXTERNAL_ORACLE=PASS`

A PASS freezes the external image oracle. It still does not authorize AIR core
image execution changes. Stages 7C-7H must derive the component/state/semantic
gap map first.


## Stage 7B final qualification

Status: CLOSED / QUALIFIED.

WolfCat evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt7B-FLUX2-Oracle-20260930-145158`

AIR source:

`636b4cf5a4d4759de8b1827dd3f3695f2371e66d`

Final markers:

- `PROMPT7B_EXTERNAL_ORACLE=PASS`;
- `PROMPT7B_FLUX2_EXTERNAL_ORACLE=PASS`;
- qualifier exit code 0.

Oracle reproducibility:

- run 1 elapsed: `10.091 s`;
- run 2 elapsed: `9.525 s`;
- output dimensions: `1024 x 1024`;
- both runs produced RGB pixel SHA-256:
  `c3a4278c608408df5019cf15162707e29263dee76e7a0114a6b1dcf2c29e1aa6`;
- RGB pixel identity: PASS;
- PNG file SHA-256 differed between runs:
  - run 1:
    `9efcc2837911a174d2ddf2568350257656584b578edcc4d68e981510829d37a7`;
  - run 2:
    `6271bad1aad04a002383043d72111893e61aef713aba1c6f700d8ff91a96834c`.

Interpretation:

The same-machine oracle comparison contract is therefore correctly defined at
the decoded RGB pixel level rather than the PNG container byte level. The
container carries non-semantic metadata that may differ even when the decoded
image is identical.

Resource evidence:

- ComfyUI reported `NORMAL_VRAM`;
- async weight offloading used 2 streams;
- DynamicVRAM was enabled;
- text encoder:
  - load device CUDA;
  - offload device CPU;
  - current state initially CPU;
  - staged size reported `3669 MB`;
- Flux2 denoiser staged size reported `3882 MB`;
- VAE:
  - load device CUDA;
  - offload device CPU;
  - staged size reported `160 MB`;
- peak observed GPU memory used: `10106 MiB`;
- minimum observed GPU memory free: `5879 MiB`;
- peak observed GPU utilization: `100%`;
- peak observed GPU temperature: `89 C`;
- peak observed GPU power: `108.15 W`;
- peak ComfyUI process RSS: `2915.15625 MiB`;
- minimum observed host available memory: `19.472991943359375 GiB`.

Model authority:

All three ComfyUI-visible component paths resolved to the same underlying
canonical files under `~/Models/Media/Image/FLUX.2-Klein-4B`. No duplicate
model authority was introduced.

The selected local published Klein-support VAE successfully decoded the oracle
output. The earlier stock-template-VAE difference is therefore not a blocking
compatibility problem for the selected oracle and remains only a retained
controlled variant.

Prompt 7B is CLOSED / QUALIFIED.

The external oracle is now frozen.

Stages 7C-7H may derive component/state/semantic requirements from this oracle.
AIR core image execution is still not authorized until that evidence-backed
gap map is complete.


## Stage 7C-H retained-oracle analysis

Prompt 7B is CLOSED / QUALIFIED.

The next step does not rerun image generation.

Authorities:

- `scripts/analyze-adaptive-prompt7c-h-flux2.py`;
- `scripts/qualify-adaptive-prompt7c-h-oracle-analysis.sh`.

Inputs:

- qualified Prompt 7B evidence directory;
- pinned ComfyUI checkout;
- current AIR source tree.

The analyzer validates the retained oracle identity and derives:

- `source-evidence.json`;
- `component-census.json`;
- `iteration-state-census.json`;
- `semantic-value-census.json`;
- `operation-boundary.json`;
- `resource-residency-census.json`;
- `current-air-gap-map.json`;
- `unresolved-evidence.json`;
- `prompt7c-h-summary.json`;
- evidence checksums.

The analysis is intentionally conservative.

It treats as facts only what is supported by:

- retained 7A/7B artifacts;
- exact pinned ComfyUI source;
- exact current AIR source;
- measured runtime/resource evidence.

It explicitly retains unknowns instead of guessing tensor representation or
transition timing.

### Expected semantic findings to test

The qualified oracle already establishes that the second workload carries
semantic concepts absent from the current token-only public/runtime vocabulary:

- conditioning;
- deterministic seed identity;
- realized noise;
- sigma schedule;
- latent state;
- decoded image.

The selected 1024 x 1024 Flux2 latent source contract is:

`[batch, 128, height/16, width/16]`

which is:

`[1, 128, 64, 64]`

for the oracle.

The pinned Flux2 scheduler source derives a 5-value sigma path for four
sampling transitions. The analyzer recomputes that schedule from the pinned
source constants and records it as evidence.

### Architectural boundary under test

Prompt 7C-H must determine what is reusable versus narrow in AIR.

Expected reusable authorities:

- hardware topology/environment;
- Strategy Lab planning authority;
- CapacityScheduler admission authority;
- PreparedModel/resource ownership;
- execution observation/evidence authority;
- one production runtime path.

Expected pressure areas:

- token-only `RequestProfile`;
- token/text-oriented `InferenceRequest` and response;
- prefill/decode-specific `ExecutionPlan` vocabulary;
- KV-specific resource accounting;
- token-specific physical invocation/payload enums;
- absence of explicit semantic image/latent/schedule/noise values;
- absence of a workload-level iterative-state contract;
- absence of per-component prepared-resource identity in runtime planning.

No core type is changed by this analysis.

### Expected unresolved evidence

The 7B oracle did not retain precise:

- per-component load latency;
- per-component eviction latency;
- exact per-phase component device residency;
- per-operation transient memory;
- per-step latent dtype/shape observations.

These remain explicit unknowns.

If the retained-oracle analysis confirms that those unknowns materially affect
Prompt 8 architecture, Prompt 7G will run one focused residency/transition
probe rather than repeating general image generation.

Expected marker:

`PROMPT7C_H_ORACLE_ANALYSIS=PASS`


## Stage 7C-H final qualification

Status: CLOSED / QUALIFIED.

WolfCat evidence:

`/home/emerson/Downloads/AIR-0.11-Prompt7C-H-FLUX2-Census-20260930-152948`

Qualified AIR source:

`04ad75e505eff5b5f0ef37830e00804b9c5f1a59`

Final marker:

`PROMPT7C_H_ORACLE_ANALYSIS=PASS`

Retained-oracle findings:

- oracle RGB pixel identity remained
  `c3a4278c608408df5019cf15162707e29263dee76e7a0114a6b1dcf2c29e1aa6`;
- latent semantic geometry is `[1, 128, 64, 64]`;
- four Flux2 sampling transitions use the five-value sigma path:
  `1.000000000, 0.967383988, 0.908143923, 0.767199964, 0.000000000`;
- text-encoder staged size: `3669 MB`;
- denoiser staged size: `3882 MB`;
- VAE staged size: `160 MB`;
- peak observed device memory from the qualified oracle remained
  `10106 MiB`;
- three unresolved evidence items were retained rather than guessed.

The gap map falsified a token-only universal architecture while preserving the
core AIR ownership model.

Reusable authorities:

- hardware topology/environment;
- Strategy Lab planning authority;
- CapacityScheduler admission authority;
- PreparedModel/resource ownership;
- ExecutionGraph/evidence concepts;
- one production runtime.

Evidence-backed pressure points:

- workload-typed request/profile data;
- semantic values beyond tokens/text;
- workload-level iterative state/control;
- identified prepared-resource/component residency;
- workload-scoped physical plan payloads;
- non-token physical invocation vocabulary;
- workload-specific output/phase metrics.

No second image runtime, planner, scheduler, capacity authority, or residency
manager is justified by the evidence.

### Retained unresolved evidence

Prompt 7C-H retained:

1. component transition/load/offload timing;
2. per-phase component residency timeline;
3. exact conditioning runtime shape/dtype only if materially required by the
   Prompt 8 semantic boundary.

The third item is not currently material.

Prompt 7 already establishes conditioning as a semantic value. Exact tensor
shape/dtype is a storage/representation property and is not required to define
the workload-typed semantic boundary. It remains explicitly unknown rather
than being measured merely because it is available to inspect.

The two material remaining targets therefore belong to one focused Stage 7G
probe.

## Stage 7G focused residency / transition observability

Status: CURRENT.

Authorities:

- `scripts/prompt7g_residency_probe.py`;
- `scripts/qualify-adaptive-prompt7g-residency.sh`;
- the unchanged qualified Prompt 7B FLUX.2 oracle driver.

### Architecture decision

Three approaches were considered.

1. Modify the pinned ComfyUI checkout with instrumentation.

   Rejected. This would contaminate the external oracle, create a forked
   measurement runtime, and risk confusing observer behavior with oracle
   behavior.

2. Observe only external GPU telemetry.

   Rejected as insufficient. Device-level memory/utilization samples cannot
   identify which text-encoder/denoiser/VAE transition caused a residency
   change.

3. Attach a source-clean launch-time observer to the pinned runtime's existing
   model-management boundaries and correlate those events with external device
   telemetry.

   Selected.

The observer preserves ComfyUI's native import/startup order. It attaches only
after the existing `comfy.model_management` and `comfy.model_patcher`
modules are loaded, then removes its import hook immediately.

It wraps existing boundaries only:

- `LoadedModel.model_load`;
- `LoadedModel.model_unload`;
- `LoadedModel.model_use_more_vram`;
- `ModelPatcher.partially_load`;
- `ModelPatcher.partially_unload`;
- `load_models_gpu`;
- `free_memory`;
- `unload_all_models`.

No ComfyUI source file is modified.

The probe reuses the exact Prompt 7B workflow and semantic comparison contract.
After the two oracle executions complete, it requests ComfyUI's existing
`POST /free` unload path to force a post-oracle eviction boundary for
measurement.

### Measurement contract

Prompt 7G must distinguish what is actually measured:

Measured:

- component-associated model-management host-call durations;
- runtime-reported loaded/model/offloaded bytes at observed boundaries;
- ordered boundary residency snapshots;
- approximately 100 ms NVIDIA device memory/utilization/power telemetry;
- unchanged output pixel identity under observation.

Not directly measured:

- exact asynchronous GPU transfer completion duration;
- exact per-layer DynamicVRAM device residency;
- kernel-level transfer/compute overlap inside the external runtime.

Those lower-level details must remain explicit external-runtime opacity. Prompt
7 does not need to reverse-engineer ComfyUI's implementation to define AIR's
own resource contracts.

### Pre-publish failure prevention

The first internal review of the probe found that importing ComfyUI
model-management before its normal CLI initialization could silently cache
default arguments and alter startup behavior.

The probe was corrected before WolfCat handoff:

- ComfyUI now initializes in native `main.py` order;
- the observer attaches after normal imports;
- the temporary import hook self-disables immediately;
- the probe records the parsed CLI contract;
- qualification fails unless deterministic mode, no-cache mode, custom-node
  disablement, and stdout logging are actually active.

This is pre-publish integration evidence, not a failed WolfCat experiment.

### Required Prompt 7G gates

1. qualified Prompt 7B and Prompt 7C-H evidence identities remain valid;
2. AIR and pinned ComfyUI worktrees remain clean;
3. the unchanged oracle retains the qualified RGB pixel identity;
4. text encoder, Flux2 denoiser, and VAE each appear at existing
   model-management load boundaries;
5. all three appear at unload/eviction boundaries after the explicit
   post-oracle free request;
6. ordered runtime residency snapshots are retained;
7. external GPU telemetry is retained;
8. observer claims remain bounded to host-call/runtime-state/device-sample
   evidence;
9. exact lower-level async/per-layer residency is reported as opaque rather
   than inferred;
10. no AIR core execution type is modified.

Expected markers:

- `PROMPT7G_RESIDENCY_TRANSITION_OBSERVABILITY=PASS`;
- `PROMPT7G_FLUX2_RESIDENCY_TRANSITION=PASS`.

### Prompt 7G exit

If the focused probe passes, Prompt 7 has enough evidence to close even if
ComfyUI's exact per-layer asynchronous residency remains opaque.

That opacity is an external-runtime implementation detail, not a missing AIR
semantic requirement.

The next step after a passing 7G probe is to derive the smallest Prompt 8
implementation packet from the qualified Qwen2 + FLUX.2 evidence.

Do not broaden Prompt 7 into deeper ComfyUI reverse engineering unless the
focused probe falsifies a material resource assumption needed by Prompt 8.


## Stage 7G first live attempt: clean-baseline precondition stop

WolfCat attempt:

`/home/emerson/Downloads/AIR-0.11-Prompt7G-FLUX2-Residency-20261001-164359`

AIR source:

`ba1b18a874b616380f7f277ec7c38204053988d4`

Result:

- retained Prompt 7 evidence validation PASS;
- qualifier stopped before starting ComfyUI or running the FLUX.2 oracle;
- existing GPU compute process:
  `/home/emerson/Projects/llama.cpp-tq3/build/bin/llama-server`;
- observed PID: `993156`;
- observed device memory: `1424 MiB`;
- qualifier exit code: `1`.

Classification:

ENVIRONMENT PRECONDITION STOP, not Prompt 7G falsification.

The clean-GPU requirement is intentional. Prompt 7G is measuring residency,
transition boundaries, and device-memory chronology. Allowing an unrelated
long-lived model server to retain VRAM would shift available capacity and could
change DynamicVRAM placement/offload behavior.

No AIR or ComfyUI code change is justified.

Required retry:

- stop or otherwise remove the unrelated GPU compute workload;
- confirm no compute process remains in the `nvidia-smi
  --query-compute-apps` baseline;
- rerun the same Prompt 7G qualifier unchanged.
