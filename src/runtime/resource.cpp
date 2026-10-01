#include "air/resource.hpp"

#include <algorithm>
#include <limits>
#include <unordered_set>

namespace air {
namespace {

std::optional<std::string_view> prepared_linear_resource_id(
    QuantizedLinearExecutionKind kind) noexcept {
    switch (kind) {
    case QuantizedLinearExecutionKind::dense_f32_cublas:
        return cuda_dense_f32_cublas_resource_id;
    case QuantizedLinearExecutionKind::q5q8_dp4a_hybrid:
        return cuda_q5q8_dp4a_hybrid_resource_id;
    case QuantizedLinearExecutionKind::baseline:
    case QuantizedLinearExecutionKind::batch_reuse4:
    case QuantizedLinearExecutionKind::batch_reuse8:
        return std::nullopt;
    }
    return std::nullopt;
}

} // namespace

const char* to_string(PreparedResourceResidencyState state) noexcept {
    switch (state) {
    case PreparedResourceResidencyState::unknown:
        return "unknown";
    case PreparedResourceResidencyState::nonresident:
        return "nonresident";
    case PreparedResourceResidencyState::resident:
        return "resident";
    }
    return "unknown";
}

Status validate_prepared_resource_requirements(
    std::span<const PreparedResourceRequirement> requirements) {
    std::unordered_set<std::string> ids;
    ids.reserve(requirements.size());
    for (const auto& requirement : requirements) {
        if (requirement.resource_id.empty()) {
            return Status::invalid_argument(
                "prepared resource requirement id must not be empty");
        }
        if (!ids.insert(requirement.resource_id).second) {
            return Status::invalid_argument(
                "prepared resource requirement ids must be unique");
        }
        if (requirement.expected_device_bytes &&
            *requirement.expected_device_bytes == 0U) {
            return Status::invalid_argument(
                "known prepared resource byte requirement must be non-zero");
        }
    }
    return Status::ok();
}

Status validate_prepared_resource_residencies(
    std::span<const PreparedResourceResidency> residencies) {
    std::unordered_set<std::string> ids;
    ids.reserve(residencies.size());
    for (const auto& residency : residencies) {
        if (residency.resource_id.empty()) {
            return Status::invalid_argument(
                "prepared resource residency id must not be empty");
        }
        if (!ids.insert(residency.resource_id).second) {
            return Status::invalid_argument(
                "prepared resource residency ids must be unique");
        }
        if (residency.state == PreparedResourceResidencyState::resident) {
            if (residency.device_bytes == 0U) {
                return Status::invalid_argument(
                    "resident prepared resource must report non-zero device bytes");
            }
        } else if (residency.device_bytes != 0U) {
            return Status::invalid_argument(
                "nonresident or unknown prepared resource must report zero resident bytes");
        }
    }
    return Status::ok();
}

Result<std::uint64_t> resident_prepared_resource_bytes(
    std::span<const PreparedResourceResidency> residencies) {
    const auto valid = validate_prepared_resource_residencies(residencies);
    if (!valid) return valid;

    std::uint64_t total = 0U;
    for (const auto& residency : residencies) {
        if (residency.state != PreparedResourceResidencyState::resident) {
            continue;
        }
        if (residency.device_bytes >
            std::numeric_limits<std::uint64_t>::max() - total) {
            return Status::data_error(
                "prepared resource resident-byte total overflow");
        }
        total += residency.device_bytes;
    }
    return total;
}

Result<bool> prepared_resources_satisfy(
    std::span<const PreparedResourceRequirement> requirements,
    std::span<const PreparedResourceResidency> residencies) {
    const auto requirement_status =
        validate_prepared_resource_requirements(requirements);
    if (!requirement_status) return requirement_status;

    const auto residency_status =
        validate_prepared_resource_residencies(residencies);
    if (!residency_status) return residency_status;

    for (const auto& requirement : requirements) {
        const auto found = std::find_if(
            residencies.begin(), residencies.end(),
            [&](const PreparedResourceResidency& residency) {
                return residency.resource_id == requirement.resource_id;
            });
        if (found == residencies.end() ||
            found->state != PreparedResourceResidencyState::resident) {
            return false;
        }
        if (requirement.expected_device_bytes &&
            found->device_bytes < *requirement.expected_device_bytes) {
            return false;
        }
    }
    return true;
}

std::vector<PreparedResourceRequirement>
prepared_resource_requirements(const ExecutionPlan& plan) {
    std::vector<PreparedResourceRequirement> out;
    if (plan.backend != BackendKind::cuda) return out;

    const QuantizedLinearExecutionKind tactics[] = {
        plan.linear.prefill_block,
        plan.linear.decode_block,
        plan.linear.decode_output,
    };

    for (const auto tactic : tactics) {
        const auto id = prepared_linear_resource_id(tactic);
        if (!id) continue;
        const bool duplicate = std::any_of(
            out.begin(), out.end(),
            [&](const PreparedResourceRequirement& requirement) {
                return requirement.resource_id == *id;
            });
        if (!duplicate) {
            out.push_back(
                PreparedResourceRequirement{std::string(*id), std::nullopt});
        }
    }
    return out;
}

} // namespace air
