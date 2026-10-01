#pragma once

#include <cstdint>
#include <string>

namespace air {

enum class PreparedResourceResidencyState {
    unknown = 0,
    nonresident,
    resident,
};

[[nodiscard]] const char* to_string(
    PreparedResourceResidencyState state) noexcept;

// Current physical residency observation. device_bytes is resident bytes, not
// nominal resource size. Nonresident/unknown observations therefore carry zero.
struct PreparedResourceResidency {
    std::string resource_id;
    PreparedResourceResidencyState state{
        PreparedResourceResidencyState::unknown};
    std::uint64_t device_bytes{0};
};

} // namespace air
