# AIR Adaptive Execution - Failure Retrospective and Prevention Rules

Status: ACTIVE PROCESS AUTHORITY
Updated: 2026-09-29

Purpose:

Every failed qualification must change either the product, the tests, or the
development process when there is a repeatable way to prevent the same class of
failure.

A failure is not closed merely because the immediate line of code was fixed.

## Prompt 1 / Prompt 2 lessons

### Failure A - qualifier appeared to stop silently

Observed:

The first Prompt 1 run stopped after the CPU-build banner while the surrounding
interactive shell printed a misleading zero status.

Root cause:

- qualification output for configure/build stages was redirected to files;
- the script did not print its own failed stage/log tail;
- the outer command captured `PIPESTATUS` after another command had already
  overwritten it.

Why it escaped review:

The success path had been designed, but the failure path of the qualifier
itself had not been tested as a product.

Permanent controls:

- every adaptive qualification script owns an ERR trap;
- every script tracks a named `STAGE`;
- failure prints stage, exit code, evidence directory, and relevant log tails;
- scripts return their own nonzero status;
- user wrappers capture `PIPESTATUS[0]` immediately after the pipeline;
- qualification harness failure behavior is part of review.

### Failure B - Boost.JSON implicit string conversion

Observed:

`air-cli machine-info --json` failed to compile against the installed
Boost.JSON version.

Root cause:

New code assumed an implicit `std::string` conversion that the qualified
toolchain/API did not provide.

Why it escaped review:

The implementation was written from API familiarity rather than matching the
exact conversion style already known to compile in this repository/toolchain.

Permanent controls:

- use explicit textual JSON construction at compatibility boundaries;
- inspect existing project patterns before introducing library-specific syntax;
- CPU preflight build is mandatory before live qualification;
- serialization code must have a parse/round-trip contract test.

### Failure C - CPU-only CUDA stub drift

Observed:

CPU-only linking failed because `CudaExecutor` had public methods with no
stub definitions.

Root cause:

The real CUDA implementation and public header had evolved while the disabled
backend stub was not kept contract-complete.

Why it escaped review:

Normal CUDA-focused qualification did not exercise the full non-CUDA link
surface.

Permanent controls:

- CPU-only build is a first-class required matrix entry;
- any backend public-interface change triggers a stub-parity audit;
- backend contract tests must link both real/stub variants when feasible;
- missing stub definitions are treated as interface-owner defects, not patched
  in unrelated callers.

### Failure D - iterator written as pointer

Observed:

`std::find_if` result was declared `const auto*`, causing compilation to
fail.

Root cause:

A simple type error was introduced during a refactor without compiler feedback.

Why it escaped review:

The source was statically reviewed for architectural ownership but not with a
dedicated compile-risk pass over newly written C++ expressions.

Permanent controls:

Before publishing a code update, explicitly inspect new/changed code for:

- iterator versus pointer/reference/value types;
- Result/Status return propagation;
- move/value lifetime;
- span/string_view lifetime;
- signed/unsigned/narrowing conversions;
- overload/default-argument mismatches;
- declarations without matching definitions.

CPU preflight must pass before asking for live qualification.

### Failure E - test target missing backend link dependency

Observed:

`air-hardware-topology-tests` compiled but failed to link
`discover_machine_hardware()` and `cuda_compiled()`.

Root cause:

The test began consuming the machine/backend contract while its CMake target
still linked only `AIR::core`.

Why it escaped review:

Source ownership was reviewed, but the build graph was not audited when the
test's dependency boundary changed.

Permanent controls:

Any new public symbol or moved ownership requires a four-part interface audit:

1. declaration owner;
2. implementation owner for every build variant;
3. each consumer/target;
4. build/link dependency that connects the consumer to the implementation.

Do not move symbols to the wrong library just to fix a linker error.

### Failure F - correlation change broke every PreparedModel test double

Observed:

Prompt 3 correlation work changed the pure-virtual signatures of
`PreparedModel::create_sequence` and `restore_sequence`. The CPU preflight
then rejected `FakePreparedModel` in scheduler tests because its existing
overrides no longer matched and the class became abstract.

Root cause:

Correlation evidence was pushed into an existing backend implementation
contract even though existing backends did not need to understand correlation
to create a sequence.

Why it escaped review:

The interface audit checked real Reference/CUDA implementations but did not
enumerate all derived implementations and test doubles before changing a pure
virtual method.

Permanent controls:

- before changing any pure virtual method, search for every derived override
  and test double first;
- prefer additive wrappers/adapters over widening a pure virtual contract when
  the new concern can be layered after the existing operation;
- treat compile impact on test fakes as architecture evidence, not test noise.

Prompt 3 was redesigned so the original pure-virtual create/restore signatures
remain unchanged. Additive overloads delegate to them and bind
`ExecutionCorrelation` to the returned `SequenceState`.

### Failure G - correlation test violated Reference test dependency isolation

Observed:

A direct correlation test added `runtime/backend.hpp` usage to
`air-reference-tests`. That target links only `AIR::core`, while
`backend.cpp` also owns CUDA-facing prepared backend code. Linking then
produced a large CUDA undefined-symbol cascade.

Root cause:

The test selected an implementation-heavy path to verify a small contract and
crossed an intentional CMake dependency boundary.

Why it escaped review:

The review focused on whether the test exercised the desired behavior, not
whether the chosen test target was the narrowest owner for that behavior.

Permanent controls:

- before adding an internal include/call to a test, inspect that test target's
  current link dependencies;
- do not widen a narrow correctness target merely for convenience;
- prefer a small dedicated fake-based contract test when the behavior can be
  tested without real backend implementations;
- a new test must state which production dependency boundary it intentionally
  exercises.

The correlation wrapper now has a dedicated core-only
`air-observation-contract-tests` target.

## Mandatory pre-publish review

Before asking a user to pull/run a new adaptive-execution code update:

1. **Diff review**
   - read every changed public header;
   - read every changed CMake target;
   - inspect every new public symbol;
   - inspect every new endpoint/serialization contract.

2. **Interface ownership audit**
   - declaration;
   - definition;
   - CPU/stub definition where required;
   - CUDA/real definition where required;
   - target linkage;
   - install/export implications.

3. **Compile-risk audit**
   - iterators/pointers;
   - Result/Status;
   - overload signatures;
   - string/JSON conversions;
   - lifetimes;
   - missing includes;
   - narrowing/conversion warnings.

4. **Build matrix**
   - CPU-only configure/build;
   - CPU-only CTest;
   - CUDA configure/build when a CUDA toolchain is available;
   - CUDA CTest when hardware/toolchain are available.

5. **Harness audit**
   - shell syntax;
   - failure stage/log output;
   - cleanup;
   - evidence path;
   - nonzero exit propagation.

6. **Architecture audit**
   - no duplicate owner;
   - no hidden fallback;
   - no new parallel runtime/planner/scheduler;
   - no semantic/physical-state conflation.

## Automation

`scripts/preflight-adaptive.sh` is the lightweight local/CI gate.

It is intentionally cheaper than prompt qualification:

- shell syntax checks;
- CPU-only fresh configure;
- CPU-only full build;
- CPU-only CTest.

GitHub Actions runs the same gate for pushes and pull requests affecting the
adaptive branch.

Live WolfCat qualification remains required for CUDA/device behavior and
machine-specific evidence.

## Behavioral rule for future agents

When a qualification failure occurs, the fixing agent must answer all four
questions before advancing:

1. What failed?
2. What owning-layer defect caused it?
3. Why did the previous review/test process fail to catch it?
4. What permanent check/process change can catch this class earlier next time?

If question 4 has a practical answer, implement it before moving to the next
prompt.
