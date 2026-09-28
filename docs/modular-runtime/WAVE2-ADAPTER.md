# AIR Modular Runtime R0 - Wave 2 Architecture Adapter

Status: IMPLEMENTED / AWAITING MACHINE QUALIFICATION

## Objective

Introduce the smallest internal model-architecture seam justified by the Wave 1 census.

This wave does not introduce a universal neural graph, new public model API, new scheduler, new backend, or new tensor-binding representation.

## Implemented seam

```text
ModelDefinition
      |
      v
resolve_model_architecture()
      |
      v
ModelArchitectureAdapter
      |
      +-- Qwen2ArchitectureAdapter
              |
              +-- validate Qwen2 structure
              +-- enumerate current execution tensors
```

Qwen2 remains the only production architecture implementation.

Unsupported architecture identifiers return an explicit `unsupported` status.

## Ownership change

Before Wave 2:

```text
ReferenceExecutor ----> qwen2_contract
CudaExecutor ---------> qwen2_contract
```

After Wave 2:

```text
ReferenceExecutor ----+
                      |
                      v
              architecture_adapter
                      |
                      v
                qwen2_contract
                      ^
                      |
CudaExecutor ---------+
```

The existing `qwen2_contract` remains the implementation of current Qwen2 semantics.

The adapter now owns architecture resolution.

## Intentionally unchanged

- `ModelDefinition` remains canonical loaded model truth.
- GGUF remains a format/import concern.
- Qwen2 remains the only executable model architecture.
- Reference numerical execution is unchanged.
- CUDA kernels and execution mechanics are unchanged.
- Qwen2/GGUF source tensor names are still used during execution.
- Scheduler, admission, SequenceState, serving, Decision, HTTP, browser, and MEF contracts are unchanged.
- No registry framework is introduced.

Source tensor-name ownership moves in Wave 3, not Wave 2.

## Regression authority

Wave 1 characterization protects:

- wrong architecture rejection;
- missing required Qwen2 tensor rejection;
- wrong required tensor shape rejection;
- optional Q/K/V bias acceptance;
- optional output bias;
- tied output behavior;
- unsupported RoPE scaling;
- unsupported sliding-window behavior;
- existing Qwen2 reference execution.

## Wave 2 exit gate

Wave 2 may close only when the exact branch HEAD:

1. configures and builds with CUDA enabled on WolfCat-Studio;
2. passes all 12 CTests;
3. preserves explicit unsupported behavior for an unknown architecture;
4. introduces no second executor or scheduler path;
5. leaves public AIR and frozen MEF R0 contracts unchanged.

If qualification fails, fix the owning defect in this seam. Do not bypass the adapter or weaken existing tests.
