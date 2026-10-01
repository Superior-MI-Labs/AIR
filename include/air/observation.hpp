#pragma once

#include "air/types.hpp"
#include "air/work_unit.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace air {

inline constexpr std::uint32_t execution_observation_schema_version = 2U;

enum class ExecutionObservationLevel {
    off = 0,
    normal,
    detailed,
};

enum class ExecutionSpanScope {
    service = 0,
    backend,
};

enum class ExecutionSpanCategory {
    queue = 0,
    preparation,
    backend_call,
    transfer,
    synchronization,
    delivery,
    request,
};

[[nodiscard]] const char* to_string(ExecutionObservationLevel level) noexcept;
[[nodiscard]] const char* to_string(ExecutionSpanScope scope) noexcept;
[[nodiscard]] const char* to_string(ExecutionSpanCategory category) noexcept;

class ExecutionObservationSink;

struct ExecutionCorrelation {
    RequestId request_id{0};
    SequenceId sequence_id{0};
    ExecutionObservationSink* sink{nullptr};
};

struct BackendExecutionObservation {
    ExecutionSpanCategory category{ExecutionSpanCategory::backend_call};
    std::string_view phase;
    std::string_view backend;
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point end;
    std::uint32_t participant_count{1};
    std::optional<WorkUnitKind> work_unit_kind;
    std::uint64_t work_units{0};
    bool success{true};
};

class ExecutionObservationSink {
public:
    virtual ~ExecutionObservationSink() = default;
    virtual void observe_backend(
        const ExecutionCorrelation& correlation,
        const BackendExecutionObservation& observation) noexcept = 0;
};

struct ExecutionSpan {
    std::uint32_t schema_version{execution_observation_schema_version};
    std::uint64_t observation_sequence{0};
    RequestId request_id{0};
    SequenceId sequence_id{0};
    ExecutionSpanScope scope{ExecutionSpanScope::service};
    ExecutionSpanCategory category{ExecutionSpanCategory::backend_call};
    std::string phase;
    std::string backend;
    std::uint64_t start_ns{0};
    std::uint64_t end_ns{0};
    std::uint32_t participant_count{1};
    std::optional<WorkUnitKind> work_unit_kind;
    std::uint64_t work_units{0};
    bool success{true};
};

struct ExecutionTimelineSnapshot {
    std::uint32_t schema_version{execution_observation_schema_version};
    ExecutionObservationLevel level{ExecutionObservationLevel::normal};
    std::uint64_t origin_unix_ms{0};
    std::uint32_t capacity{0};
    std::uint64_t evicted_spans{0};
    std::uint64_t dropped_spans{0};
    std::vector<ExecutionSpan> spans;
};

} // namespace air
