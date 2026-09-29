# AIR Adaptive Execution R0 - GUI Census

Status: WAVE 0 IN PROGRESS
Current web generation: AIR Web 3.2

## Existing authority model

The browser currently consumes canonical AIR HTTP surfaces.

The Runtime page explicitly says structured live state comes from `GET
/runtime` and that nested AIR objects stay nested rather than being flattened
into a parallel UI schema.

This is correct and should remain a hard invariant.

## Current browser state

Browser-side state currently includes:

- route;
- connection;
- health;
- model;
- runtime;
- recent events;
- Prometheus text;
- last error;
- last generation;
- bounded browser-session event history.

The browser-session event history is explicitly non-durable.

## Current views useful to retain/evolve

### Runtime

Already shows:

- queued/active/completed/rejected;
- device/KV/prepared bytes;
- planner;
- scheduler;
- capabilities;
- sequence-state store;
- raw runtime JSON.

This can evolve into the control-room Overview/Runtime area.

### Diagnostics

Already shows recent events and raw health/model evidence.

This can evolve into typed execution/event timelines once the server exposes
better evidence.

### Metrics

Already shows a human-readable projection while retaining raw Prometheus text.

Prometheus should remain operational metrics, not become the semantic execution
data model.

## Missing server-side surfaces for the target GUI

Likely future structured data:

- hardware topology snapshot;
- dynamic execution environment;
- semantic workload/program view;
- physical execution graph;
- execution timeline/spans;
- buffer/component residency;
- active plan + candidate plans;
- measurement/evidence references;
- bottleneck inference;
- unsupported/missing semantic contracts.

These should be designed server/runtime-first.

## Browser anti-patterns to reject

- deriving hardware topology from strings in JavaScript;
- calculating planner truth separately in browser code;
- reconstructing execution dependencies from event text;
- browser-owned plan IDs;
- browser-owned qualification state;
- browser-owned historical performance database;
- direct scheduler mutation;
- unbounded event accumulation.

## Wave 0 GUI next step

Before redesigning visuals:

1. map current refresh cadence and API polling;
2. define typed server snapshots/events needed by each target view;
3. define normal vs research tracing volume;
4. define reconnect semantics;
5. define bounded browser history;
6. prototype layout only after authoritative data contracts exist.
