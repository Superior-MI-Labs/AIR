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
