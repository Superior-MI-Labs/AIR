# AIR Public Roadmap

AIR is research software. This roadmap describes areas of investigation, not
promised release dates.

## Current public checkpoint: 0.10.0

The current release establishes:

- an internal architecture-adapter and prepared-semantic model boundary;
- Reference/CUDA execution independent of Qwen2 source tensor naming below preparation;
- Qwen2 as the sole qualified production model architecture;
- one production inference path;
- bounded scheduling/backpressure;
- reference and CUDA execution;
- exact sequence-state ownership;
- native generation;
- bounded semantic Decision;
- installable CMake package;
- browser command center;
- release qualification and evidence discipline.

## Active architecture research: Adaptive Execution Substrate

The next AIR architecture program is exploring a standalone adaptive execution
substrate.

The research direction separates:

```text
semantic computation
hardware topology
dynamic execution environment
physical execution plan
measured performance evidence
```

The goal is not to hard-code every future model family. It is to make new
semantics and new hardware enter through explicit contracts while AIR derives
qualified physical execution strategies from measured evidence.

Current work begins with hardware/execution assumption census and observability.
Qwen2 remains the qualified baseline. A multi-component image generation
workflow is the intended second architecture used to falsify proposed common
abstractions.

The program is documented under `docs/adaptive-execution/`.

## Near-term public research

### Long-context concurrent prefill

Release qualification showed strong short-request concurrency scaling but weak
long-context c7 throughput/TTFT behavior.

Post-release research is separating:

- scheduler prefill quantum;
- execution/microbatch shape;
- KV page geometry;
- kernel occupancy/bandwidth effects;
- fairness and TTFT coupling.

No source optimization is retained without correctness and machine evidence.

### Browser/setup experience

The public web application is being developed toward a local software-like
experience:

- system readiness;
- dependency guidance;
- model selection;
- launch/status;
- generation;
- Decision;
- runtime/diagnostics.

A future bootstrap helper must stay separate from inference and must not expose
generic shell execution.

### Wider model/hardware qualification

The current public support boundary is intentionally narrow. Broader
architecture/device support requires explicit qualification rather than
assuming all GGUF files behave equivalently.

## Deliberately deferred

- calibrated Decision confidence;
- automatic Decision scorer selection;
- abstention/multi-select thresholds;
- persistent CUDA cross-request state retention;
- fuzzy semantic execution-state reuse;
- generic response caching inside AIR.

Some of these responsibilities may belong outside AIR entirely.

## How to contribute

Useful contributions include:

- reproducible bug reports;
- portability fixes;
- characterization tests;
- independent benchmark reproduction;
- UI/diagnostic improvements;
- falsifiable performance hypotheses.

See `CONTRIBUTING.md`.
