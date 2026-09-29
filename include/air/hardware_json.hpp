#pragma once

#include "air/hardware.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace air {

[[nodiscard]] std::string hardware_topology_json(
    const HardwareTopology& topology);

[[nodiscard]] std::string hardware_environment_json(
    const HardwareEnvironmentSnapshot& environment);

[[nodiscard]] std::string hardware_discovery_json(
    const HardwareDiscovery& discovery,
    std::string_view air_version = {},
    std::optional<bool> cuda_compiled = std::nullopt);

} // namespace air
