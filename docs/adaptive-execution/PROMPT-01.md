# AIR 0.11 Strategy - Prompt 1

Status: IN PROGRESS
Title: Release contract and machine discovery foundation

## Prompt objective

Make AIR independently aware of the execution resources it directly controls
before attempting adaptive planning.

The product question is:

> Without loading a model and without depending on Builder/MEF, what does AIR
> see on this machine that it can potentially execute on?

## Verified baseline

Existing fragments:

- `HardwareTopology` v1 exists but is not a canonical live discovery surface;
- `cuda_devices()` enumerates CUDA devices and current free/total memory;
- qualification computes a Linux CPU/NVIDIA hardware digest;
- `RuntimeSnapshot` samples CUDA free memory and prepared/KV state;
- synthetic hardware fixtures already cover several system shapes;
- current GUI has no canonical machine topology endpoint/view.

## Prompt 1 implementation scope

Introduce a read-only machine discovery contract that separates:

```text
stable-ish physical topology
        from
dynamic resource availability
```

Initial qualified host implementation:

- Linux CPU identity/architecture/features;
- logical CPU count;
- host RAM total;
- current host RAM availability;
- CUDA devices when AIR was built with CUDA and devices are present;
- CUDA compute capability;
- CUDA total/free memory;
- unmeasured host-device relationships.

The first implementation may leave storage/NUMA/cache hierarchy incomplete if
the observation cannot yet be represented honestly. Missing data must be
explicit rather than invented.

## Product surface

Add:

```text
air-cli machine-info
air-cli machine-info --json
```

The human form is for quick inspection.

The JSON form is the evidence artifact used for development-machine testing and
later GUI/server work.

No model path is required.

## Authority rules

- discovery is read-only;
- discovery does not choose a model;
- discovery does not choose an external runtime/provider;
- discovery does not change scheduler policy;
- discovery does not benchmark links automatically;
- observed free memory is dynamic state, not topology identity;
- no shell execution from package metadata.

## Prompt 1 tests

Unit/characterization:

- topology structural validation;
- topology fingerprint stable when only dynamic availability changes;
- topology fingerprint changes when structural CPU/GPU identity changes;
- environment snapshot references known topology/resources;
- malformed/duplicate resource state rejected;
- Linux host discovery returns CPU + memory on supported host;
- CUDA augmentation is empty/safe in non-CUDA builds;
- JSON output is syntactically valid.

Machine qualification:

Run on WolfCat-Studio and retain:

- CLI human output;
- CLI JSON output;
- exact AIR HEAD;
- CUDA compiled/available state;
- detected CPU/RAM/GPU;
- topology fingerprint;
- environment timestamp/available memory;
- no mutation of runtime/model state.

## Prompt 1 GUI implication

Do not build the new GUI yet.

The later Machine view will consume this canonical data. Prompt 1 should make
the future GUI a projection rather than forcing JavaScript to rediscover
hardware.

## Exit gate

Prompt 1 closes only when:

1. code builds CPU-only;
2. CUDA build still passes existing tests;
3. machine discovery unit tests pass;
4. current AIR tests remain green;
5. WolfCat machine-info output is captured and matches observed hardware;
6. discovery does not affect inference behavior.

## Next prompt

Prompt 2 converts the transitional discovery contract into the final
HardwareTopology / ExecutionEnvironment authority split and exposes canonical
server read-only machine/environment state.
