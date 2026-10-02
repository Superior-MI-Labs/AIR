#pragma once

#include "air/status.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace air {

enum class SemanticRequirementKind {
    operation = 0,
    component,
};

[[nodiscard]] const char* to_string(
    SemanticRequirementKind kind) noexcept;

struct SemanticContractVersion {
    std::uint32_t major{0};
    std::uint32_t minor{0};
    std::uint32_t patch{0};

    friend bool operator==(
        const SemanticContractVersion&,
        const SemanticContractVersion&) = default;
};

struct SemanticRequirement {
    SemanticRequirementKind kind{SemanticRequirementKind::operation};
    std::string semantic_id;
    SemanticContractVersion contract_version{};
};

struct SemanticImplementationDescriptor {
    SemanticRequirement requirement;
    std::string implementation_id;
    SemanticContractVersion implementation_version{};
};

enum class TrustedImplementationOrigin {
    built_in = 0,
    host_registered,
};

[[nodiscard]] const char* to_string(
    TrustedImplementationOrigin origin) noexcept;

// An implementation object must already exist as trusted host code. Package
// metadata cannot manufacture one and AIR never interprets package strings as
// source, commands, library paths, or entrypoints.
class SemanticImplementation {
public:
    virtual ~SemanticImplementation() = default;
    [[nodiscard]] virtual SemanticImplementationDescriptor descriptor()
        const = 0;
};

enum class MissingSemanticReason {
    not_registered = 0,
    contract_version_not_registered,
};

[[nodiscard]] const char* to_string(
    MissingSemanticReason reason) noexcept;

struct MissingSemantic {
    SemanticRequirement requirement;
    MissingSemanticReason reason{
        MissingSemanticReason::not_registered};
    std::vector<SemanticContractVersion> available_versions;
};

struct SemanticResolution {
    const SemanticImplementation* implementation{nullptr};
    std::optional<TrustedImplementationOrigin> origin;
    std::optional<MissingSemantic> missing;

    [[nodiscard]] bool resolved() const noexcept {
        return implementation != nullptr && origin.has_value() &&
            !missing.has_value();
    }
};

// Data-only package declaration. It contains requirements, not executable
// extension material.
struct SemanticPackageDeclaration {
    std::string package_id;
    std::vector<SemanticRequirement> requirements;
};

struct RegisteredSemanticImplementation {
    SemanticImplementationDescriptor descriptor;
    TrustedImplementationOrigin origin{
        TrustedImplementationOrigin::built_in};
};

struct SemanticRequirementStatus {
    SemanticRequirement requirement;
    std::optional<SemanticImplementationDescriptor> implementation;
    std::optional<TrustedImplementationOrigin> origin;
    std::optional<MissingSemantic> missing;

    [[nodiscard]] bool resolved() const noexcept {
        return implementation.has_value() && origin.has_value() &&
            !missing.has_value();
    }
};

struct SemanticPackageStatus {
    std::string package_id;
    std::vector<SemanticRequirementStatus> requirements;
    std::uint32_t resolved_count{0};
    std::uint32_t missing_count{0};
};

struct SemanticRegistrySnapshot {
    std::vector<RegisteredSemanticImplementation> implementations;
    std::vector<SemanticPackageStatus> packages;
};

[[nodiscard]] Status validate_semantic_requirement(
    const SemanticRequirement& requirement);

[[nodiscard]] Status validate_semantic_package_declaration(
    const SemanticPackageDeclaration& package);

class SemanticImplementationRegistry final {
public:
    [[nodiscard]] Status register_trusted(
        std::shared_ptr<const SemanticImplementation> implementation,
        TrustedImplementationOrigin origin);

    [[nodiscard]] SemanticResolution resolve(
        const SemanticRequirement& requirement) const;

    [[nodiscard]] std::vector<SemanticResolution> resolve(
        const SemanticPackageDeclaration& package) const;

    [[nodiscard]] std::vector<SemanticImplementationDescriptor>
    descriptors() const;

    [[nodiscard]] std::vector<RegisteredSemanticImplementation>
    registered_implementations() const;

private:
    struct Entry {
        std::shared_ptr<const SemanticImplementation> implementation;
        SemanticImplementationDescriptor descriptor;
        TrustedImplementationOrigin origin{
            TrustedImplementationOrigin::built_in};
    };

    std::vector<Entry> entries_;
};

[[nodiscard]] Result<SemanticRegistrySnapshot>
snapshot_semantic_registry(
    const SemanticImplementationRegistry& registry,
    std::span<const SemanticPackageDeclaration> packages);

} // namespace air
