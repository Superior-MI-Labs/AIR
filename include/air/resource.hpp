#pragma once

#include "air/execution.hpp"
#include "air/result.hpp"
#include "air/resource_state.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace air {

// Prepared-resource identity is physical execution identity. AIR core compares
// IDs but does not infer semantic meaning from their spelling.
inline constexpr std::string_view cuda_dense_f32_cublas_resource_id =
    "cuda/linear/dense-f32-cublas";
inline constexpr std::string_view cuda_q5q8_dp4a_hybrid_resource_id =
    "cuda/linear/q5q8-dp4a-hybrid";

// A requirement names the exact prepared physical resource a plan needs.
// expected_device_bytes is evidence/forecast when known. Absence means the
// requirement identity is known but its byte cost is not asserted here.
struct PreparedResourceRequirement {
    std::string resource_id;
    std::optional<std::uint64_t> expected_device_bytes;
};

[[nodiscard]] Status validate_prepared_resource_requirements(
    std::span<const PreparedResourceRequirement> requirements);

[[nodiscard]] Status validate_prepared_resource_residencies(
    std::span<const PreparedResourceResidency> residencies);

[[nodiscard]] Result<std::uint64_t> resident_prepared_resource_bytes(
    std::span<const PreparedResourceResidency> residencies);

// Identity is mandatory. Equal anonymous byte totals are never sufficient to
// prove a prepared state is hot.
[[nodiscard]] Result<bool> prepared_resources_satisfy(
    std::span<const PreparedResourceRequirement> requirements,
    std::span<const PreparedResourceResidency> residencies);

// Prompt 8B first characterizes current Qwen optional prepared artifacts.
// This projection does not estimate bytes and does not validate plan legality.
// It only maps already-selected prepared linear tactics to stable identities.
[[nodiscard]] std::vector<PreparedResourceRequirement>
prepared_resource_requirements(const ExecutionPlan& plan);

} // namespace air
