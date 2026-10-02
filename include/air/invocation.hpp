#pragma once

#include "air/workload.hpp"

#include <cstdint>
#include <string>
#include <variant>

namespace air {

// The existing PhysicalInvocation remains the qualified autoregressive token
// invocation used by ExecutionGraph R0. This alias names that meaning without
// changing the already-qualified Qwen graph/runtime contract.
using AutoregressivePhysicalInvocation = PhysicalInvocation;

// Prompt 7 proves a finite iterative physical workload shape. This type carries
// only physical structure: finite work, concurrency, and placement. Semantic
// values such as conditioning, noise, sigma schedule, latent state, and image
// output remain outside this contract and belong to the known semantic adapter.
struct IterativePhysicalInvocation {
    std::uint64_t iteration_count{0};
    std::uint32_t active_instances{1};

    // Placement is supplied by existing AIR hardware/planning authority. The
    // invocation does not discover hardware or own placement policy.
    std::string topology_fingerprint;
    std::string hardware_resource_id;
};

// One discriminated physical-invocation boundary. It is not a second graph or
// runtime. ExecutionGraph R0 still consumes only its already-qualified
// autoregressive member until the later ExecutionGraph R1 projection stage.
using WorkloadPhysicalInvocation =
    std::variant<AutoregressivePhysicalInvocation, IterativePhysicalInvocation>;

[[nodiscard]] ExecutionWorkloadKind physical_invocation_workload_kind(
    const WorkloadPhysicalInvocation& invocation) noexcept;

[[nodiscard]] WorkUnitKind physical_invocation_work_unit_kind(
    const WorkloadPhysicalInvocation& invocation) noexcept;

// This validates only the new shared discrimination boundary. Detailed
// autoregressive prefill/decode legality remains owned by derive_execution_graph
// and its backend capability checks. The iterative member has no second
// executor/graph path here.
[[nodiscard]] Status validate_workload_physical_invocation_boundary(
    const WorkloadPhysicalInvocation& invocation);

} // namespace air
