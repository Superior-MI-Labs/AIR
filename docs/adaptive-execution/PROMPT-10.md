# Prompt 10 - AIR Control Room

Status: CURRENT
Program: AIR 0.11.0 Adaptive Execution Foundation
Baseline entering Prompt 10: `ffad7f76edf319efa5a4125ce8f03ea37ff492cc`

## Objective

Turn the existing AIR browser shell into a thin Control Room over canonical
server state.

The browser must help a human answer:

- what workload/model is loaded?
- what machine does AIR see?
- what environment capacity is currently available?
- which physical plan/strategy is selected and why?
- what prepared resources are resident?
- what execution spans/graphs were observed?
- which semantic capabilities exist?
- which semantics remain unsupported?
- what evidence is measured versus descriptive/pending?

## Authority rule

The browser owns presentation only.

It must not:

- maintain a second planner;
- infer resource residency from byte totals;
- fabricate bottleneck conclusions from incomplete evidence;
- reinterpret MissingSemantic;
- invent graph concordance;
- scan for models/providers;
- execute shell/setup commands.

## Web 3.3 data surfaces

Fast/live:

- /health
- /model
- /runtime
- /events
- /metrics

Structural / slower refresh:

- /machine
- /environment
- /semantics

Research evidence:

- /timeline
- /execution-graphs

## Views

Existing product views remain:

- Home / Overview
- Playground
- Decision
- Models
- Runtime / Memory
- Metrics
- Setup
- About

Control Room adds:

- Machine
- Execution
- Plan Lab
- Semantics / Extensions

## Novice and research modes

Novice mode:

- prioritizes status, selected plan, resource pressure, missing capabilities;
- avoids loading detailed graph/timeline evidence unless the Execution view is
  opened.

Research mode:

- loads detailed timeline/graph evidence continuously;
- exposes raw canonical JSON and structural identifiers;
- never changes runtime scheduling/execution policy.

UI mode is browser-local preference only.

## Plan Lab rule

Plan Lab renders Strategy Lab candidate traces already emitted by /runtime.

It does not calculate a winner itself.

Candidate disposition, memory feasibility, transition estimates, break-even
evidence, and selected strategy come from AIR.

## Semantics rule

The Semantics view renders /semantics directly:

- trusted registered implementations;
- package requirements;
- resolved/missing state;
- missing reason;
- exact versions.

Missing capabilities are normal structured state, not a generic error banner.

## Machine rule

Machine topology and dynamic environment remain separate in the UI.

Stable topology comes from /machine.

Available memory/capacity comes from /environment.

The UI must not merge dynamic availability into topology identity.

## Execution/evidence rule

Timeline and ExecutionGraph are shown as evidence.

The UI distinguishes graph binding:

- air-executable;
- descriptive.

It also distinguishes evidence status:

- concordant;
- incomplete;
- contradictory;
- not-evaluated.

No descriptive FLUX graph may be presented as executed.

## Hosted qualification

Prompt 10 adds web preflight to scripts/preflight-adaptive.sh:

- every index asset reference exists;
- JS syntax checked when Node is available;
- required Control Room views are registered;
- required canonical endpoint strings are present;
- boot order keeps core -> API -> views -> main.

C++ protocol tests continue to own JSON shape correctness.

## Hardware/user-test debt

Hosted qualification cannot replace:

- real RTX/CUDA graph replay;
- real browser rendering on the development machine;
- human novice/research usability observation.

Those remain explicit Prompt 11 evidence debt.

## Exit

Prompt 10 closes when:

- all Control Room views are implemented;
- Web 3.3 preflight passes;
- existing AIR tests remain green;
- no parallel authority is introduced;
- remaining human/hardware validation is explicitly handed to Prompt 11.
