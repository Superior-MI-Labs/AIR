# AIR Modular Runtime R0 - Wave 7 Full-System Qualification

Status: AIR-LOCAL CLOSED / EXTERNAL MEF R0 GATE PENDING

## Objective

Qualify the modular model-architecture boundary as part of the complete AIR system without broadening the architecture.

No production architecture changes are authorized in this wave unless qualification demonstrates a correctness or release-safety defect.

## Qualified input from Wave 6

Wave 6 closed with:

```text
Release build
CUDA=ON
12/12 CTests PASS
0 failures
```

That result includes:

- renamed-source Reference execution parity;
- renamed-source CUDA execution parity;
- dense-FP32 tactic preparation from semantic roles;
- destructive prepared-semantic validation;
- preservation of public Qwen2 source-name requirements.

## Release-qualification source drift discovered

The historical release-candidate harness and public documentation contain schema identities from earlier AIR development phases.

Live source at the Wave 7 branch defines:

```text
execution manifest schema: 10
benchmark report schema:    air.benchmark.v11
verification report schema: air.verification.v1
```

Historical/current narrative files still contain older schema numbers.

Wave 7 does not reinterpret old frozen evidence.

The machine qualification harness now derives current schema identities from source rather than hard-coding a historical schema.

Public documentation is updated consistently only during the final Wave 8 release freeze.

## Isolation rule

Wave 7 must not qualify whatever AIR happens to be installed in `~/.local`.

The qualification entrypoint:

```text
scripts/qualify-modular-runtime-r0.sh
```

requires:

- the exact `architecture/modular-runtime-r0` branch;
- a clean source worktree;
- a user-supplied supported GGUF model;
- a new isolated Release/CUDA build directory;
- a new isolated install prefix;
- all executed AIR binaries to resolve from that isolated prefix.

The exact Git HEAD, model SHA-256, model size, build path, prefix, RC archive path, and RC archive SHA-256 are retained in the Wave 7 evidence directory.

## Qualification program

The Wave 7 machine program performs the following gates.

### Build and package

- Release build;
- CUDA enabled;
- complete CTest suite;
- install into isolated prefix;
- installed binary origin verification;
- external `find_package(AIR)` consumer configure/build/run.

### Numerical execution

- Reference/CUDA differential verification on the real model;
- fresh CUDA-only qualification manifest;
- current manifest schema identity derived from source;
- previous manifest schema explicitly rejected.

### Public surfaces

- executable help/version surfaces;
- `GET /`;
- `GET /health`;
- `GET /model`;
- `GET /runtime`;
- `GET /events`;
- `GET /metrics`;
- `GET /v1/models`;
- native `POST /generate`;
- `POST /v1/completions`;
- `POST /v1/chat/completions`;
- native `POST /decide`;
- unsupported-field rejection;
- conflicting-token-field rejection.

### Browser and generation

The existing AIR web doctor is run against both:

- the exact source web tree;
- the isolated installed web tree;
- the live served application.

The existing generation doctor validates:

- non-streaming generation response text;
- streaming SSE delivery.

### Reliability and lifecycle

- concurrent generation stress;
- malformed/invalid request rejection;
- client disconnect/cancellation;
- idle resource reclamation;
- clean server shutdown;
- fresh server restart;
- deterministic one-active/one-queued overload configuration;
- synchronized request pressure;
- at least one HTTP 503 rejection;
- runtime overload counter increment;
- post-overload resource reclamation;
- clean overload-server shutdown.

## MEF compatibility is external

AIR does not own MEF qualification.

After the AIR-local Wave 7 program passes, frozen MEF R0 remains an independent external compatibility authority.

The AIR endpoint must still satisfy the frozen `provider.air.http` contract for:

```text
text.generate@1.0.0
```

using MEF tag:

```text
mef-r0-qualified
33f63246244f91acfba5659bba80447e6e181108
```

The MEF gate must not require changes to MEF provider/core semantics solely because AIR internals became modular.

## Evidence location

The qualification entrypoint creates:

```text
~/Downloads/AIR-Modular-Runtime-W7-<timestamp>/
```

containing, at minimum:

- `identity.txt`;
- isolated build tree;
- isolated install prefix;
- RC evidence directory;
- RC evidence ZIP;
- `wave7-summary.txt`.

The RC evidence includes per-gate exit codes and generated runtime artifacts.

## First machine qualification result

The first isolated WolfCat-Studio Wave 7 run successfully exercised the upgraded
qualification harness and produced a genuine single-gate failure:

```text
overall_fail=1

PASS:
installed binary origin
help contract
fresh qualification manifest
current manifest schema
legacy manifest rejection
installed CMake consumer
server startup/shutdown
bounded overload
overload resource reclamation
overload shutdown

FAIL:
differential_verification=9
```

The differential report itself remained finite and preserved top-1 parity for
all 16 teacher-forced decisions.

Observed full-logit error:

```text
requested atol:       0.001
worst max_abs_error:  0.0039185285568237305
top-1 parity:         16/16
finite:               true
```

The release harness therefore failed correctly rather than silently accepting
the result.

The historical AIR v0.9.12 RC harness uses the same `--atol 0.001` threshold.
That threshold must not be loosened without evidence.

A dedicated discriminator is now provided:

```text
scripts/compare-v0912-verification.sh
```

It builds frozen `v0.9.12` on the same machine/toolchain and runs the identical
model, prompt, teacher-forced decision count, CUDA device, top-k, and tolerance.

Observed frozen-baseline comparison on the same WolfCat-Studio machine,
model, CUDA 13.0.88 toolchain, prompt, device, and verification command:

```text
current branch:
  decisions=16
  top1_parity=true
  finite=true
  max_abs_error=0.0039185285568237305
  mean_decision_max_abs_error=0.00205797515809536
  max_rms_error=0.0008627701382458027
  strict_atol_pass=false

frozen v0.9.12:
  decisions=16
  top1_parity=true
  finite=true
  max_abs_error=0.0039185285568237305
  mean_decision_max_abs_error=0.00205797515809536
  max_rms_error=0.0008627701382458027
  strict_atol_pass=false
```

The reported numerical error distribution is identical at these summary
measurements. Therefore the modular runtime work is not the source of the
historical `0.001` failure on this current environment.

Wave 7 does not replace or hide the historical strict measurement.

The revised numerical gate records:

```text
differential_verification_strict=<raw air-verify exit>
```

and, when strict verification fails, requires an independently generated
frozen `v0.9.12` report from the same machine/model/toolchain.

The baseline-relative gate requires:

- the same verification schema;
- the same nonzero decision count;
- finite current and baseline reports;
- top-1 parity in both reports;
- identical teacher-forced token history;
- identical per-decision Reference/CUDA top-token history;
- current max, RMS, and mean absolute error no worse than the frozen baseline
  for every decision, apart from a `1e-7` serialization/numerical comparison
  epsilon.

There is no percentage or broad absolute-error allowance.

Thus `0.001` remains visible as the historical diagnostic threshold, while
release qualification asks the scientifically relevant compatibility question:
did AIR 0.10.0 make Reference/CUDA agreement worse than the frozen public
release under the same environment?

A full Wave 7 qualification rerun remains required because the first run
skipped several downstream public-surface/stress gates after the strict
differential failure.

## Second machine qualification attempt

The next isolated Wave 7 run reached the expected branch head, completed the
Release/CUDA build, passed all 12 CTests, and installed the isolated prefix.

It then stopped before the frozen-baseline comparison because the newly added
shell helper had been created without an executable file mode and the wrapper
attempted to execute it directly:

```text
scripts/compare-v0912-verification.sh: Permission denied
```

This is a qualification-harness invocation defect only. It occurred after the
product build/tests/install completed and before the numerical baseline/RC
gates ran.

Root-cause fix:

- the Wave 7 wrapper now invokes shell helpers explicitly through `bash`;
- this removes dependence on repository executable-bit metadata for the
  qualification entrypoint, baseline comparator, and RC validator;
- no production AIR source or numerical behavior changed.

A complete Wave 7 rerun remains required.

## Third machine qualification attempt

The next full isolated Wave 7 run completed the product qualification program
and reduced the remaining failure to one browser-doctor harness check.

Product and runtime results:

```text
12/12 CTests PASS

differential_verification_strict=9
differential_verification_baseline=0
differential_verification=0

qualification=0
manifest_current=0
legacy_manifest_rejected=0
cmake_consumer=0

server_ready=0
generation_doctor=0
native_decision=0
native_generate=0
openai_completions=0
openai_chat=0
unsupported_field_rejected=0
conflicting_fields_rejected=0
light_stress=0
resource_reclamation=0
server_shutdown=0
bounded_overload=0
overload_resource_reclamation=0
overload_server_shutdown=0

web_doctor=1
overall_fail=1
```

The archived browser-doctor evidence shows that both source and installed web
trees are AIR Web `3.2.0`, all 12 JavaScript files parse, all live runtime
endpoints return HTTP 200, all served `v320` script bytes return HTTP 200 and
parse successfully, and the legacy `setup.js` path is not served.

The failure is caused solely by stale expectations inside
`scripts/air-web-doctor.sh`:

```text
EXPECTED=3.1.0
expected entry=main-v310.js
actual web=3.2.0
actual entry=main-v320.js
```

Root-cause fix:

- the web doctor no longer hard-codes a historical web version;
- canonical source `web/index.html` owns the expected web version and main
  entry script for the check;
- the installed web tree must match both identities;
- the served page must match both identities;
- JavaScript parsing, live endpoint checks, legacy-path rejection, and launcher
  checks remain unchanged.

No AIR production C++, CUDA, scheduler, serving, model, or web application
bytes were changed by this fix.

Because all other Wave 7 gates already passed on the isolated product and the
remaining change is qualification-harness-only, a targeted browser requalification
against that exact isolated prefix is sufficient before AIR-local Wave 7
closure. A full product rebuild is not required unless the targeted check
exposes an actual served-application defect.

Targeted entrypoint:

```text
scripts/requalify-wave7-web.sh
```

The targeted requalifier refuses reuse if any product-affecting file changed
since the full Wave 7 product qualification head. It reuses only the exact
isolated installed prefix and qualification manifest, launches that server,
runs the corrected web doctor, records the new evidence, and requires clean
shutdown.

## Targeted browser requalification result

The targeted browser requalification against the exact isolated Wave 7
installed prefix reported:

```text
TARGETED WAVE 7 WEB REQUALIFICATION: PASS
```

The requalifier also enforced that no product-affecting file had changed since
the full Wave 7 product qualification head.

Therefore the prior `web_doctor=1` result is closed as qualification-harness
drift. The served AIR Web 3.2 application, installed web tree, source web tree,
runtime endpoints, JavaScript parsing, legacy-path rejection, and clean server
shutdown all passed the corrected doctor.

AIR-local Wave 7 is CLOSED / QUALIFIED.

The only remaining Wave 7 authority is the independent frozen MEF R0
`provider.air.http` compatibility gate.

## Wave 7 exit gate

Wave 7 may close only when:

1. the exact clean branch head completes the isolated Release/CUDA build;
2. all 12 CTests pass;
3. the machine RC validator reports `overall_fail=0`;
4. Reference/CUDA verification either satisfies the historical strict threshold or passes the frozen-v0.9.12 non-regression gate on the same machine/model/toolchain;
5. installed-package consumer passes;
6. generation, chat, Decision, browser, and observability endpoints pass;
7. malformed/unsupported protocol behavior remains explicit;
8. normal stress and disconnect tests reclaim resources;
9. bounded overload produces explicit HTTP 503 evidence and reclaims resources;
10. shutdown and restart are clean;
11. frozen MEF R0 independently requalifies AIR as `provider.air.http`;
12. no Builder, MEF, scheduler, serving, or public capability redesign is required.

If a gate fails, repair the owning defect and rerun the affected qualification. Do not broaden the architecture to make qualification pass.

If all gates pass, Wave 8 may freeze the exact release source, update the public documentation/version surface, rerun final identity/smoke checks, and publish AIR 0.10.0.
