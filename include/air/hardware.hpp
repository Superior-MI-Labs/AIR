#pragma once

#include "air/result.hpp"
#include "air/status.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace air {

inline constexpr std::uint32_t hardware_topology_schema_version = 1U;
inline constexpr std::uint32_t hardware_environment_schema_version = 1U;

enum class HardwareNodeKind {
    cpu = 0,
    host_memory,
    accelerator,
    storage,
    remote_accelerator,
};

enum class HardwareLinkKind {
    memory_access = 0,
    host_device,
    peer_device,
    storage_host,
    storage_device,
    remote,
};

[[nodiscard]] const char* to_string(HardwareNodeKind kind) noexcept;
[[nodiscard]] const char* to_string(HardwareLinkKind kind) noexcept;

struct HardwareNode {
    std::string id;
    HardwareNodeKind kind{HardwareNodeKind::cpu};
    std::string name;
    std::string backend;
    std::string architecture;
    std::int32_t ordinal{-1};
    std::int32_t numa_node{-1};
    std::uint64_t total_bytes{0};
    // Retained for AIR 0.10 source compatibility. New live discovery keeps
    // volatile availability in HardwareEnvironmentSnapshot instead of using
    // this field as topology identity.
    std::uint64_t available_bytes{0};
    std::vector<std::string> capabilities;
    std::uint32_t logical_processors{0};
};

struct HardwareLink {
    std::string source_id;
    std::string target_id;
    HardwareLinkKind kind{HardwareLinkKind::memory_access};
    // Retained for AIR 0.10 source compatibility. Empirical link performance
    // is evidence, not structural topology identity.
    bool measured{false};
    double bandwidth_bytes_per_second{0.0};
    double latency_microseconds{0.0};
};

struct HardwareTopology {
    std::uint32_t schema_version{hardware_topology_schema_version};
    std::string fingerprint;
    std::vector<HardwareNode> nodes;
    std::vector<HardwareLink> links;
};

struct HardwareResourceState {
    std::string node_id;
    std::uint64_t available_bytes{0};
};

struct HardwareEnvironmentSnapshot {
    std::uint32_t schema_version{hardware_environment_schema_version};
    std::string topology_fingerprint;
    std::uint64_t observed_unix_ms{0};
    std::vector<HardwareResourceState> resources;
};

struct HardwareDiscovery {
    HardwareTopology topology;
    HardwareEnvironmentSnapshot environment;
};

struct HardwareTopologyValidation {
    bool valid{false};
    std::string message;
};

struct HardwareEnvironmentValidation {
    bool valid{false};
    std::string message;
};

[[nodiscard]] const HardwareNode* find_hardware_node(
    const HardwareTopology& topology, std::string_view id) noexcept;

// Structural validation only. This function does not select placement,
// scheduling, execution tactics, or infer missing performance.
[[nodiscard]] HardwareTopologyValidation validate_hardware_topology(
    const HardwareTopology& topology);

// Fingerprint only structural topology. Volatile availability and empirical
// bandwidth/latency measurements are intentionally excluded.
[[nodiscard]] std::string hardware_topology_fingerprint(
    const HardwareTopology& topology);

[[nodiscard]] HardwareEnvironmentValidation validate_hardware_environment(
    const HardwareTopology& topology,
    const HardwareEnvironmentSnapshot& environment);

// Read-only discovery of stable-ish host structure AIR can directly inspect.
// The first qualified implementation is Linux CPU + host memory.
[[nodiscard]] Result<HardwareTopology> discover_host_topology();

// Read-only observation of volatile host resource availability against one
// already-discovered topology.
[[nodiscard]] Result<HardwareEnvironmentSnapshot> observe_host_environment(
    const HardwareTopology& topology);

// Compatibility composition retained for callers that want one snapshot.
// New code should keep topology discovery and environment observation
// conceptually separate.
[[nodiscard]] Result<HardwareDiscovery> discover_host_hardware();

} // namespace air
