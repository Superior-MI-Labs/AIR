#include "air/hardware.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

air::HardwareTopology laptop_fixture() {
    air::HardwareTopology topology;
    topology.fingerprint = "synthetic:laptop";
    topology.nodes = {
        {"cpu0", air::HardwareNodeKind::cpu, "synthetic cpu", "cpu", "x86_64",
         0, 0, 0, 0, {"avx2"}},
        {"ram0", air::HardwareNodeKind::host_memory, "host memory", "host", "dram",
         0, 0, 36ULL * 1024ULL * 1024ULL * 1024ULL,
         30ULL * 1024ULL * 1024ULL * 1024ULL, {"pageable", "pinned-capable"}},
        {"gpu0", air::HardwareNodeKind::accelerator, "synthetic gpu", "cuda", "sm86",
         0, -1, 16ULL * 1024ULL * 1024ULL * 1024ULL,
         12ULL * 1024ULL * 1024ULL * 1024ULL, {"graphs", "async-copy"}},
        {"nvme0", air::HardwareNodeKind::storage, "synthetic nvme", "filesystem", "nvme",
         0, -1, 1024ULL * 1024ULL * 1024ULL * 1024ULL,
         512ULL * 1024ULL * 1024ULL * 1024ULL, {"mmap"}},
    };
    topology.links = {
        {"cpu0", "ram0", air::HardwareLinkKind::memory_access, true, 40.0e9, 0.08},
        {"ram0", "gpu0", air::HardwareLinkKind::host_device, true, 12.0e9, 8.0},
        {"nvme0", "ram0", air::HardwareLinkKind::storage_host, true, 5.0e9, 80.0},
    };
    return topology;
}

} // namespace

int main() {
    auto valid = laptop_fixture();
    auto result = air::validate_hardware_topology(valid);
    require(result.valid, "valid synthetic laptop topology rejected");
    require(air::find_hardware_node(valid, "gpu0") != nullptr, "node lookup failed");
    require(air::find_hardware_node(valid, "missing") == nullptr,
            "missing node lookup should be null");

    auto duplicate = laptop_fixture();
    duplicate.nodes.push_back(duplicate.nodes.front());
    require(!air::validate_hardware_topology(duplicate).valid, "duplicate node id accepted");

    auto capacity = laptop_fixture();
    capacity.nodes[1].available_bytes = capacity.nodes[1].total_bytes + 1U;
    require(!air::validate_hardware_topology(capacity).valid,
            "impossible available capacity accepted");

    auto dangling = laptop_fixture();
    dangling.links.push_back(
        {"gpu0", "gpu1", air::HardwareLinkKind::peer_device, true, 1.0e9, 5.0});
    require(!air::validate_hardware_topology(dangling).valid, "dangling link accepted");

    auto invalid_measurement = laptop_fixture();
    invalid_measurement.links[0].bandwidth_bytes_per_second =
        std::numeric_limits<double>::quiet_NaN();
    require(!air::validate_hardware_topology(invalid_measurement).valid,
            "NaN measurement accepted");

    auto unmeasured = laptop_fixture();
    unmeasured.links.push_back(
        {"cpu0", "gpu0", air::HardwareLinkKind::remote, false, 0.0, 0.0});
    require(air::validate_hardware_topology(unmeasured).valid,
            "unmeasured capability edge rejected");

    auto fingerprint_base = laptop_fixture();
    fingerprint_base.fingerprint = air::hardware_topology_fingerprint(fingerprint_base);
    auto dynamic_only = fingerprint_base;
    dynamic_only.nodes[1].available_bytes = 1U;
    dynamic_only.links[0].bandwidth_bytes_per_second = 999.0e9;
    dynamic_only.links[0].latency_microseconds = 999.0;
    require(air::hardware_topology_fingerprint(dynamic_only) ==
                fingerprint_base.fingerprint,
            "topology fingerprint changed with dynamic/measurement-only fields");

    auto structural_change = fingerprint_base;
    structural_change.nodes[0].architecture = "different-architecture";
    require(air::hardware_topology_fingerprint(structural_change) !=
                fingerprint_base.fingerprint,
            "topology fingerprint ignored structural identity change");

    air::HardwareEnvironmentSnapshot environment;
    environment.topology_fingerprint = fingerprint_base.fingerprint;
    environment.observed_unix_ms = 1U;
    environment.resources = {
        {"ram0", 12ULL * 1024ULL * 1024ULL * 1024ULL},
        {"gpu0", 8ULL * 1024ULL * 1024ULL * 1024ULL},
    };
    require(air::validate_hardware_environment(fingerprint_base, environment).valid,
            "valid hardware environment rejected");

    auto duplicate_environment = environment;
    duplicate_environment.resources.push_back(duplicate_environment.resources.front());
    require(!air::validate_hardware_environment(
                 fingerprint_base, duplicate_environment).valid,
            "duplicate hardware environment resource accepted");

    auto unknown_environment = environment;
    unknown_environment.resources.push_back({"missing", 1U});
    require(!air::validate_hardware_environment(
                 fingerprint_base, unknown_environment).valid,
            "unknown hardware environment resource accepted");

    auto stale_environment = environment;
    stale_environment.topology_fingerprint = "hardware-topology:v1:stale";
    require(!air::validate_hardware_environment(
                 fingerprint_base, stale_environment).valid,
            "stale hardware environment topology identity accepted");

#if defined(__linux__)
    auto discovered = air::discover_host_hardware();
    require(static_cast<bool>(discovered), "Linux host discovery failed");
    require(air::validate_hardware_topology(discovered.value().topology).valid,
            "Linux discovered topology failed validation");
    require(air::validate_hardware_environment(
                discovered.value().topology, discovered.value().environment).valid,
            "Linux discovered environment failed validation");
    require(air::find_hardware_node(discovered.value().topology, "cpu0") != nullptr,
            "Linux discovery did not report CPU");
    require(air::find_hardware_node(discovered.value().topology, "ram0") != nullptr,
            "Linux discovery did not report host memory");
#endif

    auto multigpu = laptop_fixture();
    multigpu.nodes.push_back(
        {"gpu1", air::HardwareNodeKind::accelerator, "second gpu", "cuda", "sm90",
         1, -1, 80ULL * 1024ULL * 1024ULL * 1024ULL,
         70ULL * 1024ULL * 1024ULL * 1024ULL, {"graphs", "peer-access"}});
    multigpu.links.push_back(
        {"gpu0", "gpu1", air::HardwareLinkKind::peer_device, true, 50.0e9, 2.0});
    require(air::validate_hardware_topology(multigpu).valid,
            "valid multi-gpu graph rejected");

    std::cout << "hardware topology tests PASS\n";
    return 0;
}
