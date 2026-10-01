#include "air/workload.hpp"

#include <type_traits>

namespace air {

const char* to_string(WorkloadKind kind) noexcept {
    switch (kind) {
    case WorkloadKind::autoregressive_tokens:
        return "autoregressive-tokens";
    case WorkloadKind::iterative_state:
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

WorkloadKind workload_kind(const WorkloadRequestProfile& profile) noexcept {
    return std::visit(
        [](const auto& value) noexcept {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, AutoregressiveRequestProfile>) {
                return WorkloadKind::autoregressive_tokens;
            } else {
                return WorkloadKind::iterative_state;
            }
        },
        profile);
}

WorkUnitKind work_unit_kind(const WorkloadRequestProfile& profile) noexcept {
    return workload_kind(profile) == WorkloadKind::autoregressive_tokens
        ? WorkUnitKind::tokens
        : WorkUnitKind::iterations;
}

std::uint32_t active_workload_instances(
    const WorkloadRequestProfile& profile) noexcept {
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
    const WorkloadRequestProfile& profile) noexcept {
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
