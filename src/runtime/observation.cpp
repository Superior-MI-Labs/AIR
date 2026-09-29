#include "air/observation.hpp"

namespace air {

const char* to_string(ExecutionObservationLevel level) noexcept {
    switch (level) {
    case ExecutionObservationLevel::off: return "off";
    case ExecutionObservationLevel::normal: return "normal";
    case ExecutionObservationLevel::detailed: return "detailed";
    }
    return "unknown";
}

const char* to_string(ExecutionSpanScope scope) noexcept {
    switch (scope) {
    case ExecutionSpanScope::service: return "service";
    case ExecutionSpanScope::backend: return "backend";
    }
    return "unknown";
}

const char* to_string(ExecutionSpanCategory category) noexcept {
    switch (category) {
    case ExecutionSpanCategory::queue: return "queue";
    case ExecutionSpanCategory::preparation: return "preparation";
    case ExecutionSpanCategory::backend_call: return "backend-call";
    case ExecutionSpanCategory::transfer: return "transfer";
    case ExecutionSpanCategory::synchronization: return "synchronization";
    case ExecutionSpanCategory::delivery: return "delivery";
    case ExecutionSpanCategory::request: return "request";
    }
    return "unknown";
}

} // namespace air
