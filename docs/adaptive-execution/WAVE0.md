# AIR Adaptive Execution R0 - Wave 0

Status: CURRENT
Type: architecture census / characterization / documentation
Production behavior changes: NOT AUTHORIZED by default

## Purpose

Wave 0 prevents a large, context-driven rewrite.

Before AIR treats hardware, execution, and timing as first-class data, we need
to identify exactly where those concerns already live and which existing
abstractions can be evolved.

## Workstreams

### A. Model/semantic census

Inventory:

- `ModelConfig` assumptions;
- `PreparedModelSemantics`;
- architecture adapter;
- tokenizer/chat assumptions;
- Reference/CUDA preparation;
- generation/Decision APIs;
- model-specific source checks.

Classify:

- artifact/format;
- architecture semantic;
- operation semantic;
- workload semantic;
- physical execution;
- serving-only.

### B. Hardware/environment census

Inventory:

- `HardwareTopology`;
- topology tests;
- CUDA device discovery;
- memory queries;
- driver/runtime identity;
- current `RuntimeSnapshot`;
- qualification hardware identity.

Classify each field:

- structural fact;
- capability;
- dynamic state;
- measurement;
- inference;
- policy.

### C. Timing/dataflow census

Trace:

- request submit;
- queue;
- admission;
- tokenization/preparation;
- model preparation;
- host-device transfers;
- prefill;
- decode;
- synchronization;
- streaming;
- completion;
- cancellation;
- shutdown.

Record which intervals are directly measured and which are currently inferred.

### D. Scheduler/execution census

Map:

- capacity scheduler;
- microbatch scheduler;
- `ExecutionPlan`;
- tactics;
- plan preparation;
- sequence state;
- KV ownership;
- prefix state;
- backend calls.

Identify the boundary between:

- semantic workload scheduling;
- runtime fairness/admission;
- physical device scheduling.

### E. Browser/observability census

Map every current browser view to the authoritative server/runtime data that
feeds it.

Identify:

- missing typed snapshots;
- duplicate derived calculations in JavaScript;
- polling/event behavior;
- reconnect behavior;
- performance overhead.

## Characterization tests to add when gaps are found

Candidate tests include:

- hardware topology identity/validation;
- explicit dynamic-state separation;
- runtime snapshot provenance;
- planner determinism;
- plan legality independent of candidate order;
- event/timestamp monotonicity;
- bounded observation queues;
- no browser-owned runtime truth.

Do not add tests merely to bless proposed architecture.

## Deliverables

Wave 0 closes with:

1. `ASSUMPTION-CENSUS.md`;
2. `EVIDENCE-CENSUS.md`;
3. `GUI-CENSUS.md`;
4. characterization tests for material unprotected current behavior;
5. a refined Wave 1 hardware/environment contract;
6. updated `CURRENT.md`.

## Exit gate

Wave 1 may start only when we can answer:

- what current AIR structures will be evolved;
- what will be deleted/replaced;
- what remains intentionally transformer-specific;
- what hardware fields are facts versus dynamic state/measurement;
- where timing observations originate;
- which current scheduler remains authoritative;
- how the GUI will observe without becoming an owner.

No universal semantic IR and no second architecture implementation are
authorized in Wave 0.
