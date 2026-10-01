#include "air/workload.hpp"

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

const char* to_string(WorkUnitKind kind) noexcept {
    switch (kind) {
    case WorkUnitKind::tokens:
        return "tokens";
    case WorkUnitKind::iterations:
        return "iterations";
    }
    return "unknown";
}

ExecutionWorkloadKind execution_workload_kind(const ExecutionWorkloadProfile& profile) noexcept {
    return std::visit(
        [](const auto& value) noexcept {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, AutoregressiveRequestProfile>) {
                return ExecutionWorkloadKind::autoregressive_tokens;
            } else {
                return ExecutionWorkloadKind::iterative_state;
            }
        },
        profile);
}

WorkUnitKind work_unit_kind(const ExecutionWorkloadProfile& profile) noexcept {
    return execution_workload_kind(profile) == ExecutionWorkloadKind::autoregressive_tokens
        ? WorkUnitKind::tokens
        : WorkUnitKind::iterations;
}

std::uint32_t active_workload_instances(
    const ExecutionWorkloadProfile& profile) noexcept {
    return std::visit(
        [](const auto& value) noexcept -> std::uint32_t {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, AutoregressiveRequestProfile>) {
                return value.active_sequences;
            } else {
                return value.active_instances;
            }
        },
        profile);
}

Status validate_workload_request_profile(
    const ExecutionWorkloadProfile& profile) noexcept {
    if (const auto* iterative = std::get_if<IterativeRequestProfile>(&profile)) {
        if (iterative->iteration_count == 0U) {
            return Status::invalid_argument(
                "iterative workload profile requires non-zero iteration_count");
        }
        if (iterative->active_instances == 0U) {
            return Status::invalid_argument(
                "iterative workload profile requires non-zero active_instances");
        }
    }
    return Status::ok();
}

} // namespace air
