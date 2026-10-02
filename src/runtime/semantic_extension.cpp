#include "air/semantic_extension.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <utility>

namespace air {
namespace {

bool valid_identity(const std::string& value) {
    if (value.empty()) return false;
    if (value.front() == '/' || value.back() == '/' ||
        value.find("..") != std::string::npos ||
        value.find("//") != std::string::npos) {
        return false;
    }
    return std::all_of(
        value.begin(), value.end(),
        [](unsigned char c) {
            return std::isalnum(c) != 0 ||
                c == '.' || c == '/' || c == '_' || c == '-';
        });
}

std::string requirement_key(
    const SemanticRequirement& requirement) {
    return std::string(to_string(requirement.kind)) + ":" +
        requirement.semantic_id + ":" +
        std::to_string(requirement.contract_version.major) + "." +
        std::to_string(requirement.contract_version.minor) + "." +
        std::to_string(requirement.contract_version.patch);
}

} // namespace

const char* to_string(SemanticRequirementKind kind) noexcept {
    switch (kind) {
    case SemanticRequirementKind::operation:
        return "operation";
    case SemanticRequirementKind::component:
        return "component";
    }
    return "unknown";
}

const char* to_string(
    TrustedImplementationOrigin origin) noexcept {
    switch (origin) {
    case TrustedImplementationOrigin::built_in:
        return "built-in";
    case TrustedImplementationOrigin::host_registered:
        return "host-registered";
    }
    return "unknown";
}

const char* to_string(MissingSemanticReason reason) noexcept {
    switch (reason) {
    case MissingSemanticReason::not_registered:
        return "not-registered";
    case MissingSemanticReason::contract_version_not_registered:
        return "contract-version-not-registered";
    }
    return "unknown";
}

Status validate_semantic_requirement(
    const SemanticRequirement& requirement) {
    if (!valid_identity(requirement.semantic_id)) {
        return Status::invalid_argument(
            "semantic requirement id is empty or contains unsupported characters");
    }
    return Status::ok();
}

Status validate_semantic_package_declaration(
    const SemanticPackageDeclaration& package) {
    if (!valid_identity(package.package_id)) {
        return Status::invalid_argument(
            "semantic package id is empty or contains unsupported characters");
    }

    std::unordered_set<std::string> requirements;
    requirements.reserve(package.requirements.size());
    for (const auto& requirement : package.requirements) {
        const auto status =
            validate_semantic_requirement(requirement);
        if (!status) return status;

        if (!requirements.insert(
                requirement_key(requirement)).second) {
            return Status::invalid_argument(
                "semantic package requirements must be unique");
        }
    }
    return Status::ok();
}

Status SemanticImplementationRegistry::register_trusted(
    std::shared_ptr<const SemanticImplementation> implementation,
    TrustedImplementationOrigin origin) {
    if (!implementation) {
        return Status::invalid_argument(
            "trusted semantic implementation must not be null");
    }

    auto descriptor = implementation->descriptor();
    const auto requirement_status =
        validate_semantic_requirement(descriptor.requirement);
    if (!requirement_status) return requirement_status;
    if (!valid_identity(descriptor.implementation_id)) {
        return Status::invalid_argument(
            "semantic implementation id is empty or contains unsupported characters");
    }

    const auto duplicate = std::find_if(
        entries_.begin(), entries_.end(),
        [&](const Entry& entry) {
            return entry.descriptor.requirement.kind ==
                    descriptor.requirement.kind &&
                entry.descriptor.requirement.semantic_id ==
                    descriptor.requirement.semantic_id &&
                entry.descriptor.requirement.contract_version ==
                    descriptor.requirement.contract_version;
        });
    if (duplicate != entries_.end()) {
        return Status::invalid_state(
            "a trusted implementation is already registered for this exact semantic contract");
    }

    entries_.push_back(
        Entry{
            std::move(implementation),
            std::move(descriptor),
            origin,
        });
    return Status::ok();
}

SemanticResolution SemanticImplementationRegistry::resolve(
    const SemanticRequirement& requirement) const {
    SemanticResolution result;
    const auto requirement_status =
        validate_semantic_requirement(requirement);
    if (!requirement_status) {
        result.missing = MissingSemantic{
            requirement,
            MissingSemanticReason::not_registered,
            {},
        };
        return result;
    }

    std::vector<SemanticContractVersion> available;
    for (const auto& entry : entries_) {
        if (entry.descriptor.requirement.kind != requirement.kind ||
            entry.descriptor.requirement.semantic_id !=
                requirement.semantic_id) {
            continue;
        }

        available.push_back(
            entry.descriptor.requirement.contract_version);
        if (entry.descriptor.requirement.contract_version ==
            requirement.contract_version) {
            result.implementation = entry.implementation.get();
            result.origin = entry.origin;
            return result;
        }
    }

    std::sort(
        available.begin(), available.end(),
        [](const auto& lhs, const auto& rhs) {
            if (lhs.major != rhs.major) return lhs.major < rhs.major;
            if (lhs.minor != rhs.minor) return lhs.minor < rhs.minor;
            return lhs.patch < rhs.patch;
        });
    available.erase(
        std::unique(available.begin(), available.end()),
        available.end());

    result.missing = MissingSemantic{
        requirement,
        available.empty()
            ? MissingSemanticReason::not_registered
            : MissingSemanticReason::contract_version_not_registered,
        std::move(available),
    };
    return result;
}

std::vector<SemanticResolution>
SemanticImplementationRegistry::resolve(
    const SemanticPackageDeclaration& package) const {
    std::vector<SemanticResolution> out;
    const auto package_status =
        validate_semantic_package_declaration(package);
    if (!package_status) return out;

    out.reserve(package.requirements.size());
    for (const auto& requirement : package.requirements) {
        out.push_back(resolve(requirement));
    }
    return out;
}

std::vector<SemanticImplementationDescriptor>
SemanticImplementationRegistry::descriptors() const {
    std::vector<SemanticImplementationDescriptor> out;
    out.reserve(entries_.size());
    for (const auto& entry : entries_) {
        out.push_back(entry.descriptor);
    }
    return out;
}

} // namespace air


namespace air {

std::vector<RegisteredSemanticImplementation>
SemanticImplementationRegistry::registered_implementations() const {
    std::vector<RegisteredSemanticImplementation> out;
    out.reserve(entries_.size());
    for (const auto& entry : entries_) {
        out.push_back(
            RegisteredSemanticImplementation{
                entry.descriptor,
                entry.origin,
            });
    }
    return out;
}

Result<SemanticRegistrySnapshot> snapshot_semantic_registry(
    const SemanticImplementationRegistry& registry,
    std::span<const SemanticPackageDeclaration> packages) {
    SemanticRegistrySnapshot snapshot;
    snapshot.implementations =
        registry.registered_implementations();
    snapshot.packages.reserve(packages.size());

    for (const auto& package : packages) {
        const auto package_status =
            validate_semantic_package_declaration(package);
        if (!package_status) return package_status;

        const auto resolutions = registry.resolve(package);
        if (resolutions.size() != package.requirements.size()) {
            return Status::invalid_state(
                "semantic registry package resolution did not preserve requirement cardinality");
        }

        SemanticPackageStatus status;
        status.package_id = package.package_id;
        status.requirements.reserve(package.requirements.size());

        for (std::size_t i = 0; i < package.requirements.size(); ++i) {
            SemanticRequirementStatus requirement_status;
            requirement_status.requirement = package.requirements[i];
            const auto& resolution = resolutions[i];
            if (resolution.resolved()) {
                requirement_status.implementation =
                    resolution.implementation->descriptor();
                requirement_status.origin = resolution.origin;
                ++status.resolved_count;
            } else {
                requirement_status.missing = resolution.missing;
                ++status.missing_count;
            }
            status.requirements.push_back(
                std::move(requirement_status));
        }

        snapshot.packages.push_back(std::move(status));
    }

    return snapshot;
}

} // namespace air
