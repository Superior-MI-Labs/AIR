#pragma once

#include "air/hardware.hpp"
#include "air/result.hpp"

namespace air {

// Discover the stable-ish machine structure AIR can directly execute on.
// This does not intentionally sample volatile availability.
[[nodiscard]] Result<HardwareTopology> discover_machine_topology();

// Observe volatile execution-resource availability against one structural
// topology snapshot.
[[nodiscard]] Result<HardwareEnvironmentSnapshot> observe_machine_environment(
    const HardwareTopology& topology);

// Convenience composition for callers that need both views together.
[[nodiscard]] Result<HardwareDiscovery> discover_machine_hardware();

} // namespace air
