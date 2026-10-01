#pragma once

#include "air/status.hpp"

#include <cstdint>
#include <optional>

namespace air {

// Work-unit identity is a physical/measurement interpretation, not semantic
// meaning. Values are added only when qualified AIR evidence already uses that
// count. Prompt 8A proved tokens and iterations; Prompt 8C found that existing
// CUDA transfer observations already count bytes.
enum class WorkUnitKind {
    tokens = 0,
    iterations,
    bytes,
};

[[nodiscard]] const char* to_string(WorkUnitKind kind) noexcept;

// A zero count may legitimately have no unit (for example a synchronization
// span). Every non-zero count must be interpretable and therefore typed.
[[nodiscard]] Status validate_work_measure(
    std::optional<WorkUnitKind> kind,
    std::uint64_t count) noexcept;

} // namespace air
