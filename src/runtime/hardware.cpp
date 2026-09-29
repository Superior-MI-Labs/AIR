#include "air/hardware.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <tuple>
#include <unordered_set>

#if defined(__linux__)
#include <sys/utsname.h>
#include <unistd.h>
#endif

namespace air {
namespace {

std::string trim(std::string value) {
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string value_after_colon(const std::string& line) {
    const auto pos = line.find(':');
    return pos == std::string::npos ? std::string{} : trim(line.substr(pos + 1U));
}

std::uint64_t meminfo_bytes(std::string_view key) {
#if defined(__linux__)
    std::ifstream in("/proc/meminfo");
    if (!in) return 0U;
    std::string line;
    const std::string prefix = std::string(key) + ":";
    while (std::getline(in, line)) {
        if (line.rfind(prefix, 0) != 0) continue;
        std::istringstream stream(line.substr(prefix.size()));
        std::uint64_t kib = 0U;
        stream >> kib;
        if (!stream) return 0U;
        if (kib > std::numeric_limits<std::uint64_t>::max() / 1024U) return 0U;
        return kib * 1024U;
    }
#else
    (void)key;
#endif
    return 0U;
}

struct CpuObservation {
    std::string name;
    std::string architecture;
    std::vector<std::string> capabilities;
    std::uint32_t logical_processors{0};
};

CpuObservation observe_cpu() {
    CpuObservation out;
#if defined(__linux__)
    struct utsname uts {};
    if (uname(&uts) == 0) out.architecture = uts.machine;

    std::ifstream in("/proc/cpuinfo");
    std::string line;
    std::string features;
    while (std::getline(in, line)) {
        if (out.name.empty() &&
            (line.rfind("model name", 0) == 0 ||
             line.rfind("Hardware", 0) == 0 ||
             line.rfind("Processor", 0) == 0)) {
            out.name = value_after_colon(line);
        }
        if (features.empty() &&
            (line.rfind("flags", 0) == 0 || line.rfind("Features", 0) == 0)) {
            features = value_after_colon(line);
        }
        if (!out.name.empty() && !features.empty()) break;
    }

    if (!features.empty()) {
        std::istringstream stream(features);
        std::string feature;
        while (stream >> feature) out.capabilities.push_back(std::move(feature));
        std::sort(out.capabilities.begin(), out.capabilities.end());
        out.capabilities.erase(
            std::unique(out.capabilities.begin(), out.capabilities.end()),
            out.capabilities.end());
    }

    const long logical = sysconf(_SC_NPROCESSORS_ONLN);
    if (logical > 0 &&
        static_cast<unsigned long>(logical) <=
            static_cast<unsigned long>(std::numeric_limits<std::uint32_t>::max())) {
        out.logical_processors = static_cast<std::uint32_t>(logical);
    }
#endif
    if (out.name.empty()) out.name = "host cpu";
    if (out.architecture.empty()) out.architecture = "unknown";
    return out;
}

class Fnv1a64 {
public:
    void text(std::string_view value) {
        for (const unsigned char byte : value) {
            value_ ^= byte;
            value_ *= prime;
        }
        value_ ^= 0xffU;
        value_ *= prime;
    }

    template <typename T>
    void scalar(T value) {
        text(std::to_string(value));
    }

    [[nodiscard]] std::string digest() const {
        std::ostringstream out;
        out << "hardware-topology:v1:" << std::hex << std::setfill('0')
            << std::setw(16) << value_;
        return out.str();
    }

private:
    static constexpr std::uint64_t offset = 14695981039346656037ULL;
    static constexpr std::uint64_t prime = 1099511628211ULL;
    std::uint64_t value_{offset};
};

} // namespace

const char* to_string(HardwareNodeKind kind) noexcept {
    switch (kind) {
    case HardwareNodeKind::cpu: return "cpu";
    case HardwareNodeKind::host_memory: return "host-memory";
    case HardwareNodeKind::accelerator: return "accelerator";
    case HardwareNodeKind::storage: return "storage";
    case HardwareNodeKind::remote_accelerator: return "remote-accelerator";
    }
    return "unknown";
}

const char* to_string(HardwareLinkKind kind) noexcept {
    switch (kind) {
    case HardwareLinkKind::memory_access: return "memory-access";
    case HardwareLinkKind::host_device: return "host-device";
    case HardwareLinkKind::peer_device: return "peer-device";
    case HardwareLinkKind::storage_host: return "storage-host";
    case HardwareLinkKind::storage_device: return "storage-device";
    case HardwareLinkKind::remote: return "remote";
    }
    return "unknown";
}

const HardwareNode* find_hardware_node(
    const HardwareTopology& topology, std::string_view id) noexcept {
    for (const auto& node : topology.nodes) {
        if (node.id == id) return &node;
    }
    return nullptr;
}

HardwareTopologyValidation validate_hardware_topology(const HardwareTopology& topology) {
    if (topology.schema_version != hardware_topology_schema_version) {
        return {false, "unsupported hardware topology schema version"};
    }
    if (topology.nodes.empty()) {
        return {false, "hardware topology has no nodes"};
    }

    std::unordered_set<std::string> ids;
    ids.reserve(topology.nodes.size());
    for (const auto& node : topology.nodes) {
        if (node.id.empty()) return {false, "hardware topology node has empty id"};
        if (!ids.insert(node.id).second) {
            return {false, "hardware topology contains duplicate node id: " + node.id};
        }
        if (node.total_bytes != 0U && node.available_bytes > node.total_bytes) {
            return {false, "hardware topology node available bytes exceed total bytes: " + node.id};
        }
    }

    for (const auto& link : topology.links) {
        if (link.source_id.empty() || link.target_id.empty()) {
            return {false, "hardware topology link has empty endpoint"};
        }
        if (link.source_id == link.target_id) {
            return {false, "hardware topology link endpoints must differ"};
        }
        if (!ids.contains(link.source_id) || !ids.contains(link.target_id)) {
            return {false, "hardware topology link references an unknown node"};
        }
        if (!std::isfinite(link.bandwidth_bytes_per_second) ||
            !std::isfinite(link.latency_microseconds) ||
            link.bandwidth_bytes_per_second < 0.0 ||
            link.latency_microseconds < 0.0) {
            return {false, "hardware topology link contains invalid measurement"};
        }
        if (link.measured && link.bandwidth_bytes_per_second <= 0.0) {
            return {false, "measured hardware topology link requires positive bandwidth"};
        }
    }
    return {true, "ok"};
}

std::string hardware_topology_fingerprint(const HardwareTopology& topology) {
    Fnv1a64 hash;
    hash.scalar(topology.schema_version);

    std::vector<const HardwareNode*> nodes;
    nodes.reserve(topology.nodes.size());
    for (const auto& node : topology.nodes) nodes.push_back(&node);
    std::sort(nodes.begin(), nodes.end(), [](const auto* left, const auto* right) {
        return left->id < right->id;
    });

    for (const auto* node : nodes) {
        hash.text(node->id);
        hash.scalar(static_cast<int>(node->kind));
        hash.text(node->name);
        hash.text(node->backend);
        hash.text(node->architecture);
        hash.scalar(node->ordinal);
        hash.scalar(node->numa_node);
        hash.scalar(node->total_bytes);
        hash.scalar(node->logical_processors);

        auto capabilities = node->capabilities;
        std::sort(capabilities.begin(), capabilities.end());
        capabilities.erase(
            std::unique(capabilities.begin(), capabilities.end()),
            capabilities.end());
        for (const auto& capability : capabilities) hash.text(capability);
    }

    std::vector<const HardwareLink*> links;
    links.reserve(topology.links.size());
    for (const auto& link : topology.links) links.push_back(&link);
    std::sort(links.begin(), links.end(), [](const auto* left, const auto* right) {
        return std::tie(left->source_id, left->target_id, left->kind) <
               std::tie(right->source_id, right->target_id, right->kind);
    });

    for (const auto* link : links) {
        hash.text(link->source_id);
        hash.text(link->target_id);
        hash.scalar(static_cast<int>(link->kind));
    }

    return hash.digest();
}

HardwareEnvironmentValidation validate_hardware_environment(
    const HardwareTopology& topology,
    const HardwareEnvironmentSnapshot& environment) {
    const auto topology_valid = validate_hardware_topology(topology);
    if (!topology_valid.valid) {
        return {false, "environment references invalid topology: " + topology_valid.message};
    }
    if (environment.schema_version != hardware_environment_schema_version) {
        return {false, "unsupported hardware environment schema version"};
    }
    if (environment.topology_fingerprint.empty() ||
        environment.topology_fingerprint != topology.fingerprint ||
        environment.topology_fingerprint != hardware_topology_fingerprint(topology)) {
        return {false, "hardware environment topology fingerprint mismatch"};
    }
    if (environment.observed_unix_ms == 0U) {
        return {false, "hardware environment has no observation timestamp"};
    }

    std::unordered_set<std::string> resource_ids;
    resource_ids.reserve(environment.resources.size());
    for (const auto& resource : environment.resources) {
        if (resource.node_id.empty()) {
            return {false, "hardware environment resource has empty node id"};
        }
        if (!resource_ids.insert(resource.node_id).second) {
            return {false, "hardware environment contains duplicate resource state: " +
                           resource.node_id};
        }
        const auto* node = find_hardware_node(topology, resource.node_id);
        if (!node) {
            return {false, "hardware environment references unknown node: " +
                           resource.node_id};
        }
        if (node->total_bytes != 0U && resource.available_bytes > node->total_bytes) {
            return {false, "hardware environment available bytes exceed resource total: " +
                           resource.node_id};
        }
    }

    return {true, "ok"};
}

Result<HardwareTopology> discover_host_topology() {
#if !defined(__linux__)
    return Status::unsupported(
        "AIR host topology discovery is currently implemented only for Linux");
#else
    HardwareTopology topology;

    const auto cpu = observe_cpu();
    const auto total_memory = meminfo_bytes("MemTotal");
    if (total_memory == 0U) {
        return Status::io_error(
            "AIR could not read host memory capacity from /proc/meminfo");
    }

    topology.nodes.push_back(HardwareNode{
        "cpu0",
        HardwareNodeKind::cpu,
        cpu.name,
        "cpu",
        cpu.architecture,
        0,
        -1,
        0U,
        0U,
        cpu.capabilities,
        cpu.logical_processors,
    });
    topology.nodes.push_back(HardwareNode{
        "ram0",
        HardwareNodeKind::host_memory,
        "host memory",
        "host",
        "dram",
        0,
        0,
        total_memory,
        0U,
        {"pageable"},
        0U,
    });
    topology.links.push_back(HardwareLink{
        "cpu0",
        "ram0",
        HardwareLinkKind::memory_access,
        false,
        0.0,
        0.0,
    });

    topology.fingerprint = hardware_topology_fingerprint(topology);

    const auto valid = validate_hardware_topology(topology);
    if (!valid.valid) {
        return Status::internal_error(
            "AIR discovered invalid host topology: " + valid.message);
    }
    return topology;
#endif
}

Result<HardwareEnvironmentSnapshot> observe_host_environment(
    const HardwareTopology& topology) {
#if !defined(__linux__)
    (void)topology;
    return Status::unsupported(
        "AIR host environment observation is currently implemented only for Linux");
#else
    const auto topology_valid = validate_hardware_topology(topology);
    if (!topology_valid.valid) {
        return Status::invalid_argument(
            "cannot observe environment for invalid topology: " +
            topology_valid.message);
    }
    if (topology.fingerprint != hardware_topology_fingerprint(topology)) {
        return Status::invalid_argument(
            "cannot observe environment for stale topology fingerprint");
    }

    HardwareEnvironmentSnapshot environment;
    environment.topology_fingerprint = topology.fingerprint;
    environment.observed_unix_ms =
        static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

    const auto* host_memory = std::find_if(
        topology.nodes.begin(), topology.nodes.end(),
        [](const HardwareNode& node) {
            return node.kind == HardwareNodeKind::host_memory;
        });
    if (host_memory == topology.nodes.end()) {
        return Status::invalid_argument(
            "host environment observation requires a host-memory node");
    }

    const auto available_memory = meminfo_bytes("MemAvailable");
    environment.resources.push_back(
        HardwareResourceState{host_memory->id, available_memory});

    const auto environment_valid =
        validate_hardware_environment(topology, environment);
    if (!environment_valid.valid) {
        return Status::internal_error(
            "AIR observed invalid host environment: " +
            environment_valid.message);
    }
    return environment;
#endif
}

Result<HardwareDiscovery> discover_host_hardware() {
    auto topology = discover_host_topology();
    if (!topology) return topology.status();

    auto environment = observe_host_environment(topology.value());
    if (!environment) return environment.status();

    return HardwareDiscovery{
        std::move(topology).value(),
        std::move(environment).value(),
    };
}

} // namespace air
