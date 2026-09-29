#include "air/hardware_json.hpp"

#include <boost/json.hpp>

namespace air {
namespace json = boost::json;
namespace {

json::object node_json(const HardwareNode& node) {
    json::object out;
    out["id"] = json::value(node.id);
    out["kind"] = json::value(to_string(node.kind));
    out["name"] = json::value(node.name);
    out["backend"] = json::value(node.backend);
    out["architecture"] = json::value(node.architecture);
    out["ordinal"] = node.ordinal;
    out["numa_node"] = node.numa_node;
    out["total_bytes"] = node.total_bytes;
    out["logical_processors"] = node.logical_processors;

    json::array capabilities;
    for (const auto& capability : node.capabilities) {
        capabilities.push_back(json::value(capability));
    }
    out["capabilities"] = std::move(capabilities);
    return out;
}

json::object link_json(const HardwareLink& link) {
    json::object out;
    out["source_id"] = json::value(link.source_id);
    out["target_id"] = json::value(link.target_id);
    out["kind"] = json::value(to_string(link.kind));
    out["measured"] = link.measured;
    if (link.measured) {
        out["bandwidth_bytes_per_second"] = link.bandwidth_bytes_per_second;
        out["latency_microseconds"] = link.latency_microseconds;
    }
    return out;
}

json::object topology_object(const HardwareTopology& topology) {
    json::object out;
    out["schema_version"] = topology.schema_version;
    out["fingerprint"] = json::value(topology.fingerprint);

    json::array nodes;
    for (const auto& node : topology.nodes) {
        nodes.push_back(node_json(node));
    }
    out["nodes"] = std::move(nodes);

    json::array links;
    for (const auto& link : topology.links) {
        links.push_back(link_json(link));
    }
    out["links"] = std::move(links);
    return out;
}

json::object environment_object(const HardwareEnvironmentSnapshot& environment) {
    json::object out;
    out["schema_version"] = environment.schema_version;
    out["topology_fingerprint"] = json::value(environment.topology_fingerprint);
    out["observed_unix_ms"] = environment.observed_unix_ms;

    json::array resources;
    for (const auto& resource : environment.resources) {
        json::object item;
        item["node_id"] = json::value(resource.node_id);
        item["available_bytes"] = resource.available_bytes;
        resources.push_back(std::move(item));
    }
    out["resources"] = std::move(resources);
    return out;
}

} // namespace

std::string hardware_topology_json(const HardwareTopology& topology) {
    return json::serialize(topology_object(topology));
}

std::string hardware_environment_json(
    const HardwareEnvironmentSnapshot& environment) {
    return json::serialize(environment_object(environment));
}

std::string hardware_discovery_json(
    const HardwareDiscovery& discovery,
    std::string_view air_version,
    std::optional<bool> cuda_compiled) {
    json::object root;
    if (!air_version.empty()) {
        root["air_version"] = json::value(air_version);
    }
    if (cuda_compiled.has_value()) {
        root["cuda_compiled"] = *cuda_compiled;
    }
    root["topology"] = topology_object(discovery.topology);
    root["environment"] = environment_object(discovery.environment);
    return json::serialize(root);
}

} // namespace air
