#pragma once

#include "air/execution.hpp"
#include "air/status.hpp"

#include <cstdint>
#include <variant>

namespace air {

// Prompt 8A execution-structure identity is intentionally limited to the two
// physical workload shapes that have qualified evidence. This is separate from
// air::WorkloadKind in decision.hpp, which classifies service semantics such as
// generation versus bounded decision. Both service semantics can lower to the
// same autoregressive execution structure. Prompt 9 owns unknown-semantics
// extension behavior.
enum class ExecutionWorkloadKind {
    autoregressive_tokens = 0,
    iterative_state,
};

enum class WorkUnitKind {
    tokens = 0,
    iterations,
};

[[nodiscard]] const char* to_string(ExecutionWorkloadKind kind) noexcept;
[[nodiscard]] const char* to_string(WorkUnitKind kind) noexcept;

// Existing RequestProfile remains the qualified autoregressive request shape.
// The alias names that meaning explicitly without breaking existing callers.
using AutoregressiveRequestProfile = RequestProfile;

// Qualified FLUX.2 evidence requires a finite iterative workload shape that is
// not expressed in token vocabulary. Semantic values such as conditioning,
// noise, sigma schedule, and latent state remain owned by the workload adapter;
// they do not enter this physical planning profile.
struct IterativeRequestProfile {
    std::uint32_t iteration_count{0};
    std::uint32_t active_instances{1};
};

// A discriminated execution-planning boundary prevents token-only optional
// fields from becoming a pseudo-universal request schema.
using ExecutionWorkloadProfile =
    std::variant<AutoregressiveRequestProfile, IterativeRequestProfile>;

[[nodiscard]] ExecutionWorkloadKind execution_workload_kind(
    const ExecutionWorkloadProfile& profile) noexcept;

[[nodiscard]] WorkUnitKind work_unit_kind(
    const ExecutionWorkloadProfile& profile) noexcept;

// Active workload concurrency is the one request-shape dimension already
// shared by both qualified workloads. The helper does not imply that sequence
// and image-instance scheduling are otherwise equivalent.
[[nodiscard]] std::uint32_t active_workload_instances(
    const ExecutionWorkloadProfile& profile) noexcept;

// Validation is deliberately narrow. Existing autoregressive RequestProfile
// behavior is preserved rather than retroactively constrained. The new
// iterative profile requires a real finite iteration and at least one active
// workload instance.
[[nodiscard]] Status validate_execution_workload_profile(
    const ExecutionWorkloadProfile& profile) noexcept;

} // namespace air
