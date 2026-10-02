# Prompt 9 - Unknown Semantics / Trusted Extension Protocol

Status: CURRENT
Program: AIR 0.11.0 Adaptive Execution Foundation
Baseline entering Prompt 9: `829cb9644ab23503bc47f7cb569f92274683d2db`

## Objective

Allow an unknown semantic requirement to fail structurally and allow a trusted
implementation to be admitted without editing unrelated AIR planner/runtime
code.

This prompt must not create a general plugin/script loader.

## Security rule

Package metadata is data.

It may declare semantic requirements.

It may not declare executable:

- source code;
- scripts;
- shell commands;
- entrypoints;
- dynamic-library paths;
- arbitrary Python modules.

An executable semantic implementation must already exist as trusted host code
and be explicitly registered by AIR/the host.

## Contract

A semantic requirement is identified by:

- kind: operation or component;
- semantic ID;
- exact contract version.

Prompt 9 does not guess semantic-version compatibility.

A registered implementation has:

- the exact semantic requirement it implements;
- a stable implementation ID;
- its own implementation version;
- an explicit registration origin:
  built-in or host-registered.

## MissingSemantic

Resolution failure is structured data, not only a prose error.

It retains:

- exact required semantic kind/ID/version;
- reason:
  - not registered;
  - requested contract version not registered;
- available exact versions when applicable.

## Registry authority

There is one `SemanticImplementationRegistry`.

Registration rejects a second owner for the same exact semantic
kind/ID/version.

Package resolution never auto-loads code. It only compares data against
already-registered implementations.

## Falsification tests

1. unknown requirement produces structured MissingSemantic;
2. exact version resolves;
3. version mismatch does not silently choose another version;
4. available versions remain inspectable;
5. a test-only implementation can be registered without editing AIR core;
6. duplicate exact semantic ownership is rejected;
7. a package resolves each requirement independently;
8. package declaration has no script/source/command/entrypoint/library path
   fields;
9. executable-looking package IDs are rejected as invalid identity data;
10. all existing AIR tests remain green.

## Exit

Prompt 9 closes when hosted exact-head qualification proves the registry and
test extension contract.

Dynamic shared-library loading, package signing, trust stores, and third-party
binary distribution are explicitly outside AIR 0.11 Prompt 9.
