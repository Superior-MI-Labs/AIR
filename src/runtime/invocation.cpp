#include "air/invocation.hpp"

#include <limits>
#include <type_traits>

namespace air {

const char* to_string(ExecutionWorkloadKind kind) noexcept {
    switch (kind) {
    case ExecutionWorkloadKind::autoregressive_tokens:
        return "autoregressive-tokens";
    case ExecutionWorkloadKind::iterative_state:
        return "iterative-state";
    }
    return "unknown";
}

const char* to_string(PhysicalInvocationKind kind) noexcept {
    switch (kind) {
    case PhysicalInvocationKind::prefill_single: return "prefill-single";
    case PhysicalInvocationKind::prefill_native_batch: return "prefill-native-batch";
    case PhysicalInvocationKind::decode_single: return "decode-single";
    case PhysicalInvocationKind::decode_native_greedy_batch:
        return "decode-native-greedy-batch";
    }
    return "unknown";
}

const char* to_string(PhysicalOutputMode mode) noexcept {
    switch (mode) {
    case PhysicalOutputMode::discard: return "discard";
    case PhysicalOutputMode::logits: return "logits";
    case PhysicalOutputMode::greedy: return "greedy";
    case PhysicalOutputMode::target_logprobs: return "target-logprobs";
    }
    return "unknown";
}

ExecutionWorkloadKind physical_invocation_workload_kind(
    const WorkloadPhysicalInvocation& invocation) noexcept {
    return std::visit(
        [](const auto& value) noexcept {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, AutoregressivePhysicalInvocation>) {
                return ExecutionWorkloadKind::autoregressive_tokens;
            } else {
                return ExecutionWorkloadKind::iterative_state;
            }
        },
        invocation);
}

WorkUnitKind physical_invocation_work_unit_kind(
    const WorkloadPhysicalInvocation& invocation) noexcept {
    return physical_invocation_workload_kind(invocation) ==
            ExecutionWorkloadKind::autoregressive_tokens
        ? WorkUnitKind::tokens
        : WorkUnitKind::iterations;
}

Status validate_workload_physical_invocation_boundary(
    const WorkloadPhysicalInvocation& invocation) {
    return std::visit(
        [](const auto& value) -> Status {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, AutoregressivePhysicalInvocation>) {
                // The discriminated member itself proves autoregressive shape.
                // Keep only the Stage 8C unit invariant here. Participant/output
                // and backend-specific legality remain with ExecutionGraph R0.
                if (!value.work_unit_kind ||
                    *value.work_unit_kind != WorkUnitKind::tokens) {
                    return Status::invalid_argument(
                        "autoregressive physical invocation requires token work units");
                }
                return Status::ok();
            } else {
                if (value.iteration_count == 0U) {
                    return Status::invalid_argument(
                        "iterative physical invocation requires at least one iteration");
                }
                if (value.active_instances == 0U) {
                    return Status::invalid_argument(
                        "iterative physical invocation requires at least one active instance");
                }
                if (value.topology_fingerprint.empty()) {
                    return Status::invalid_argument(
                        "iterative physical invocation requires topology identity");
                }
                if (value.hardware_resource_id.empty()) {
                    return Status::invalid_argument(
                        "iterative physical invocation requires hardware resource identity");
                }
                if (value.iteration_count >
                    std::numeric_limits<std::uint64_t>::max() /
                        static_cast<std::uint64_t>(value.active_instances)) {
                    return Status::invalid_argument(
                        "iterative physical invocation total work overflows");
                }
                return Status::ok();
            }
        },
        invocation);
}

} // namespace air
