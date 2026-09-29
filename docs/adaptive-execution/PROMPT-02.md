# AIR 0.11 Strategy - Prompt 2

Status: IN PROGRESS
Title: Hardware topology / execution environment authority split

## Prompt objective

Turn Prompt 1 machine discovery into a stable runtime authority boundary:

```text
HardwareTopology
    = structural machine facts/capabilities

HardwareEnvironmentSnapshot
    = volatile state observed against one topology
```

The product questions are:

> What machine structure does AIR believe exists?

and independently:

> What is the current execution environment on that machine?

## Prompt 1 qualified baseline

WolfCat-Studio Prompt 1 qualified successfully.

Qualified Prompt 1 source:
`15cc24f3946707dbe2ee9843d9e8379c77b08e1e`

Observed CPU-only topology:

- Intel Core i7-11800H;
- x86_64;
- 16 logical processors;
- 31.08 GiB host memory;
- topology fingerprint
  `hardware-topology:v1:963f1082652c41fb`.

Observed CUDA topology:

- same CPU and host memory;
- NVIDIA GeForce RTX 3080 Laptop GPU;
- CUDA architecture `sm86`;
- 15.61 GiB visible device memory;
- concurrent kernels;
- async copy;
- unified addressing;
- managed memory;
- topology fingerprint
  `hardware-topology:v1:f521e9bdd95ed645`.

Both CPU-only and CUDA builds passed 12/12 CTests.

## Authority split

Prompt 2 introduces explicit discovery/observation operations.

Candidate API shape:

```text
discover_host_topology()
observe_host_environment(topology)

augment_topology_with_cuda(topology)
augment_environment_with_cuda(topology, environment)

discover_machine_hardware()
    compatibility composition of the above
```

The exact function names may differ, but ownership must not.

## Invariants

- topology fingerprint excludes current free RAM/VRAM;
- topology does not claim measured bandwidth/latency without measurement;
- environment is immutable once returned;
- environment is bound to one topology fingerprint;
- environment cannot refer to unknown resources;
- server/browser never reconstruct topology independently;
- no scheduling policy changes;
- no automatic benchmarking;
- no external-provider discovery.

## Shared serialization

CLI and server must use the same canonical JSON serialization for:

- topology;
- environment;
- combined discovery.

The JSON representation is an observability contract, not a second state owner.

## Server surfaces

Add read-only endpoints:

```text
GET /machine
GET /environment
```

`/machine` returns structural topology only.

`/environment` returns a fresh volatile observation and the topology
fingerprint it belongs to.

These are additive endpoints. Existing 0.10 public endpoints remain unchanged.

## Tests

### Unit

- host topology validates independently;
- host environment validates against host topology;
- CUDA topology augmentation does not write volatile free bytes into topology;
- CUDA environment augmentation references known accelerator node;
- topology fingerprint unchanged across repeated environment observations;
- environment rejects stale topology identity;
- shared JSON serialization is valid and stable in shape.

### CPU-only build

- 12/12 current tests pass;
- `machine-info --json` reports CPU/RAM only;
- server `/machine` reports no accelerator;
- server `/environment` reports RAM state.

### CUDA build

- 12/12 current tests pass;
- server `/machine` reports RTX 3080 Laptop / sm86 on WolfCat;
- server `/environment` reports RAM + VRAM state;
- repeated environment requests may change availability while machine
  fingerprint remains stable.

## GUI implication

Prompt 2 does not redesign the browser.

It establishes the exact server contracts the future Machine view will consume.

The browser must never derive machine topology from `/runtime`, strings,
Prometheus text, or JavaScript heuristics once these endpoints exist.

## Exit gate

Prompt 2 closes when:

1. topology/environment discovery responsibilities are separated in source;
2. CLI uses the composed canonical discovery path;
3. CLI/server share one JSON serializer;
4. `GET /machine` and `GET /environment` are qualified;
5. CPU-only and CUDA tests remain green;
6. WolfCat server output matches Prompt 1 machine evidence;
7. repeated environment observation does not mutate topology identity;
8. no planner/scheduler/inference behavior changes.

## Next prompt

Prompt 3 adds typed execution observations and a physical request timeline while
measuring observer overhead.
