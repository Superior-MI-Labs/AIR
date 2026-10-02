#pragma once

#include "air/status.hpp"
#include "air/work_unit.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace air {

// Physical workload structure is independent of service semantics such as
// generation versus bounded decision.
enum class ExecutionWorkloadKind {
    autoregressive_tokens = 0,
    iterative_state,
};

[[nodiscard]] const char* to_string(ExecutionWorkloadKind kind) noexcept;

enum class PhysicalInvocationKind {
    prefill_single = 0,
    prefill_native_batch,
    decode_single,
    decode_native_greedy_batch,
};

enum class PhysicalOutputMode {
    discard = 0,
    logits,
    greedy,
    target_logprobs,
};

[[nodiscard]] const char* to_string(PhysicalInvocationKind kind) noexcept;
[[nodiscard]] const char* to_string(PhysicalOutputMode mode) noexcept;

struct PhysicalInvocationParticipant {
    std::uint64_t work_units{0};
    PhysicalOutputMode output{PhysicalOutputMode::logits};
    std::uint32_t target_count{0};
};

struct PhysicalInvocation {
    PhysicalInvocationKind kind{PhysicalInvocationKind::prefill_single};
    std::optional<WorkUnitKind> work_unit_kind;
    std::vector<PhysicalInvocationParticipant> participants;
    std::string topology_fingerprint;
    std::string hardware_resource_id;
};

using AutoregressivePhysicalInvocation = PhysicalInvocation;

struct IterativePhysicalInvocation {
    std::uint64_t iteration_count{0};
    std::uint32_t active_instances{1};
    std::string topology_fingerprint;
    std::string hardware_resource_id;
};

using WorkloadPhysicalInvocation =
    std::variant<AutoregressivePhysicalInvocation, IterativePhysicalInvocation>;

[[nodiscard]] ExecutionWorkloadKind physical_invocation_workload_kind(
    const WorkloadPhysicalInvocation& invocation) noexcept;

[[nodiscard]] WorkUnitKind physical_invocation_work_unit_kind(
    const WorkloadPhysicalInvocation& invocation) noexcept;

[[nodiscard]] Status validate_workload_physical_invocation_boundary(
    const WorkloadPhysicalInvocation& invocation);

} // namespace air
