#include "air/work_unit.hpp"

namespace air {
namespace {

bool valid_work_unit_kind(WorkUnitKind kind) noexcept {
    switch (kind) {
    case WorkUnitKind::tokens:
    case WorkUnitKind::iterations:
    case WorkUnitKind::bytes:
        return true;
    }
    return false;
}

} // namespace

const char* to_string(WorkUnitKind kind) noexcept {
    switch (kind) {
    case WorkUnitKind::tokens:
        return "tokens";
    case WorkUnitKind::iterations:
        return "iterations";
    case WorkUnitKind::bytes:
        return "bytes";
    }
    return "unknown";
}

Status validate_work_measure(
    std::optional<WorkUnitKind> kind,
    std::uint64_t count) noexcept {
    if (kind && !valid_work_unit_kind(*kind)) {
        return Status::invalid_argument("work-unit kind is unknown");
    }
    if (count != 0U && !kind) {
        return Status::invalid_argument(
            "non-zero work count requires explicit work-unit kind");
    }
    return Status::ok();
}

} // namespace air
