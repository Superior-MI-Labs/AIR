# AIR Architecture

AIR 0.11 separates semantic computation, immutable resources, stable machine
topology, dynamic environment state, physical planning, runtime execution, and
measured evidence.

## Authority model

```text
Model / Workflow Package
          |
          v
Semantic contracts
"what must happen"
          |
          +-------------------------+
          |                         |
          v                         v
Immutable resources         HardwareTopology
                                  +
                         ExecutionEnvironment
                                  +
                           retained evidence
          |                         |
          +-------------+-----------+
                        v
              physical planning
                        |
                        v
                ExecutionGraph R1
"how this machine is expected to do it"
                        |
                        v
                 one AIR runtime
                        |
                        v
                 measured evidence
```

No layer may silently become a second owner for another layer's truth.

## Production Qwen path

```text
GGUF
  -> ModelDefinition
  -> Qwen2 Architecture Adapter
  -> PreparedModelSemantics
  -> ExecutionPlan / ExecutionGraph R1
  -> Reference or CUDA prepared backend
  -> SequenceState
  -> InferenceService
```

`ModelDefinition` remains canonical model truth. Prepared semantics and backend
resources are derived state.

## Workload structure

AIR now distinguishes service semantics from physical workload structure.

Service semantics include generation and bounded Decision.

Execution workload structure currently includes:

- autoregressive token recurrence;
- iterative state transformation.

Physical invocation is discriminated rather than forcing iterative work into
prefill/decode vocabulary.

## Work and resources

Non-zero measured work has an explicit unit:

- tokens;
- iterations;
- bytes.

Prepared resources use identity-aware requirements and residency. Equal byte
counts from different resources are never interchangeable evidence.

RuntimeSnapshot consumes identified residency. Aggregate prepared bytes are
derived diagnostics/capacity evidence, not a second mutable truth.

## Machine authority

`HardwareTopology` represents stable structural facts and has a deterministic
fingerprint.

`ExecutionEnvironmentSnapshot` represents dynamic observations such as
available memory and current device state.

Hardware fact, measurement, inference, and policy remain distinct.

## ExecutionGraph R1

The graph is immutable derived physical state, not semantic truth and not a
second executor.

It can represent both qualified workload structures and carries:

- workload-scoped physical invocation;
- optional autoregressive state;
- typed workload work;
- item multiplicity where a count is not workload progress;
- placement;
- dependencies;
- resource/implementation identities;
- opaque semantic value identities;
- binding status.

Qwen graphs can be AIR-executable. FLUX.2 graphs in 0.11 are descriptive where
required semantic/model implementations are missing.

## Second architecture: FLUX.2 Klein

The external Prompt 7 oracle established a structurally different workload:

- text conditioning;
- positive/negative conditioning values;
- latent state;
- seed/noise;
- sigma schedule;
- four iterative denoising transitions;
- VAE decode;
- independent text-encoder, denoiser, and VAE resources.

AIR reuses the same workload/resource/invocation/graph authorities rather than
creating a diffusion runtime.

AIR 0.11 itself executes the deterministic latent-geometry and schedule
semantics. Model-component execution remains a structured missing-semantic
boundary.

## Semantic extension authority

One trusted registry resolves exact semantic kind/ID/version requirements.

Unknown semantics return structured MissingSemantic state.

Packages may declare requirements but cannot execute arbitrary code. Trusted
implementations are registered by AIR or explicitly trusted extension code.

## Runtime and scheduling

`InferenceService` remains the production boundary used by HTTP, benchmark,
qualification, and Decision.

The capacity scheduler owns admission. The micro-scheduler owns ordering.
Prepared backends own physical execution state. The browser owns none of these.

## Observation and adaptation

Execution spans distinguish service/backend scope and measured phase/category.

Strategy Lab compares evidence-backed candidates under explicit objectives. It
may change physical execution but not semantic meaning.

Negative experiments remain retained evidence.

## Control Room

Web 3.3 is a thin projection over canonical server endpoints. Disconnecting the
browser cannot change runtime correctness.

## 0.11 qualification boundary

Hosted exact-source qualification covers the CPU/reference installed product,
Control Room assets, canonical read-only surfaces, generation/Decision,
ExecutionGraph R1 reference observation, semantic registry state, restart, and
external CMake consumption.

Final exact-source post-R1 CUDA replay, AIR-owned FLUX component execution/image
parity, NVIDIA thermal/power characterization, and final human usability remain
explicit evidence debt.
