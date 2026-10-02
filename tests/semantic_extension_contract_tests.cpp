#include "air/semantic_extension.hpp"
#include "air/flux2_klein_extension.hpp"

#include <concepts>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

template <typename T>
concept HasScript = requires(T value) { value.script; };

template <typename T>
concept HasSource = requires(T value) { value.source; };

template <typename T>
concept HasCommand = requires(T value) { value.command; };

template <typename T>
concept HasEntrypoint = requires(T value) { value.entrypoint; };

template <typename T>
concept HasLibraryPath = requires(T value) { value.library_path; };

class TestNormalizeSemantic final : public air::SemanticImplementation {
public:
    [[nodiscard]] air::SemanticImplementationDescriptor descriptor()
        const override {
        return {
            {
                air::SemanticRequirementKind::operation,
                "test.normalize",
                {1U, 0U, 0U},
            },
            "test.builtin.normalize",
            {1U, 0U, 0U},
        };
    }
};

class TestComponentSemantic final : public air::SemanticImplementation {
public:
    [[nodiscard]] air::SemanticImplementationDescriptor descriptor()
        const override {
        return {
            {
                air::SemanticRequirementKind::component,
                "test.component",
                {2U, 1U, 0U},
            },
            "test.host.component",
            {3U, 0U, 0U},
        };
    }
};

void test_unknown_requirement_is_structured_missing_semantic() {
    air::SemanticImplementationRegistry registry;
    const air::SemanticRequirement requirement{
        air::SemanticRequirementKind::operation,
        "test.missing",
        {1U, 2U, 3U},
    };

    const auto resolution = registry.resolve(requirement);
    check(
        !resolution.resolved() &&
        resolution.implementation == nullptr &&
        resolution.missing.has_value(),
        "unknown semantic returns structured missing result");
    if (resolution.missing) {
        check(
            resolution.missing->requirement.semantic_id ==
                requirement.semantic_id &&
            resolution.missing->requirement.contract_version ==
                requirement.contract_version &&
            resolution.missing->reason ==
                air::MissingSemanticReason::not_registered &&
            resolution.missing->available_versions.empty(),
            "missing semantic preserves exact required identity/version");
    }
}

void test_trusted_test_extension_registers_without_core_edit() {
    air::SemanticImplementationRegistry registry;
    auto implementation =
        std::make_shared<const TestNormalizeSemantic>();

    check(
        registry.register_trusted(
            implementation,
            air::TrustedImplementationOrigin::built_in).is_ok(),
        "compiled test extension can be admitted through registry");

    const auto requirement = implementation->descriptor().requirement;
    const auto resolution = registry.resolve(requirement);
    check(
        resolution.resolved() &&
        resolution.implementation == implementation.get() &&
        resolution.origin ==
            air::TrustedImplementationOrigin::built_in,
        "exact semantic requirement resolves to trusted implementation");

    check(
        !registry.register_trusted(
            implementation,
            air::TrustedImplementationOrigin::built_in).is_ok(),
        "exact semantic contract cannot acquire competing hidden owner");
}

void test_version_mismatch_is_not_guessed() {
    air::SemanticImplementationRegistry registry;
    auto implementation =
        std::make_shared<const TestNormalizeSemantic>();
    check(
        registry.register_trusted(
            implementation,
            air::TrustedImplementationOrigin::built_in).is_ok(),
        "version fixture registers");

    auto incompatible = implementation->descriptor().requirement;
    incompatible.contract_version = {2U, 0U, 0U};
    const auto resolution = registry.resolve(incompatible);
    check(
        !resolution.resolved() && resolution.missing.has_value(),
        "unregistered semantic version remains unresolved");
    if (resolution.missing) {
        check(
            resolution.missing->reason ==
                air::MissingSemanticReason::
                    contract_version_not_registered &&
            resolution.missing->available_versions ==
                std::vector<air::SemanticContractVersion>{
                    {1U, 0U, 0U}},
            "resolution exposes available exact versions without compatibility guessing");
    }
}

void test_package_resolution_is_data_only_and_complete() {
    static_assert(!HasScript<air::SemanticPackageDeclaration>);
    static_assert(!HasSource<air::SemanticPackageDeclaration>);
    static_assert(!HasCommand<air::SemanticPackageDeclaration>);
    static_assert(!HasEntrypoint<air::SemanticPackageDeclaration>);
    static_assert(!HasLibraryPath<air::SemanticPackageDeclaration>);

    air::SemanticImplementationRegistry registry;
    auto operation =
        std::make_shared<const TestNormalizeSemantic>();
    auto component =
        std::make_shared<const TestComponentSemantic>();

    check(
        registry.register_trusted(
            operation,
            air::TrustedImplementationOrigin::built_in).is_ok() &&
        registry.register_trusted(
            component,
            air::TrustedImplementationOrigin::host_registered).is_ok(),
        "mixed trusted implementation origins register explicitly");

    air::SemanticPackageDeclaration package;
    package.package_id = "test.package";
    package.requirements = {
        operation->descriptor().requirement,
        component->descriptor().requirement,
        {
            air::SemanticRequirementKind::operation,
            "test.other",
            {1U, 0U, 0U},
        },
    };

    check(
        air::validate_semantic_package_declaration(package).is_ok(),
        "data-only semantic package declaration validates");

    const auto resolutions = registry.resolve(package);
    check(
        resolutions.size() == 3U &&
        resolutions[0].resolved() &&
        resolutions[1].resolved() &&
        !resolutions[2].resolved() &&
        resolutions[2].missing.has_value(),
        "package resolution returns one result for every declared requirement");
}

void test_invalid_or_duplicate_data_is_rejected() {
    air::SemanticPackageDeclaration duplicate;
    duplicate.package_id = "test.package";
    duplicate.requirements = {
        {
            air::SemanticRequirementKind::operation,
            "test.same",
            {1U, 0U, 0U},
        },
        {
            air::SemanticRequirementKind::operation,
            "test.same",
            {1U, 0U, 0U},
        },
    };
    check(
        !air::validate_semantic_package_declaration(duplicate).is_ok(),
        "duplicate exact package requirements are rejected");

    air::SemanticPackageDeclaration executable_looking;
    executable_looking.package_id = "../run.sh";
    check(
        !air::validate_semantic_package_declaration(
            executable_looking).is_ok(),
        "package identity is not interpreted as a filesystem/script path");
}

} // namespace

void test_flux2_package_exposes_resolved_and_missing_semantics() {
    air::SemanticImplementationRegistry registry;
    check(
        air::register_flux2_klein_builtin_semantics(registry).is_ok(),
        "FLUX.2 built-in deterministic semantics register");

    const auto package =
        air::flux2_klein_semantic_package_declaration();
    const std::array packages{package};
    auto snapshot =
        air::snapshot_semantic_registry(registry, packages);
    check(snapshot.is_ok(), "FLUX.2 semantic registry snapshot derives");
    if (!snapshot) return;

    check(
        snapshot.value().implementations.size() == 2U &&
        snapshot.value().packages.size() == 1U,
        "snapshot exposes two AIR-owned FLUX semantic implementations");
    if (!snapshot.value().packages.empty()) {
        const auto& status = snapshot.value().packages.front();
        check(
            status.package_id == "flux2-klein.oracle-v1" &&
            status.requirements.size() == 11U &&
            status.resolved_count == 2U &&
            status.missing_count == 9U,
            "FLUX package truthfully exposes implemented versus missing semantic surface");

        bool schedule_resolved = false;
        bool denoiser_missing = false;
        for (const auto& requirement : status.requirements) {
            if (requirement.requirement.semantic_id ==
                "flux2-klein.derive-schedule") {
                schedule_resolved =
                    requirement.resolved() &&
                    requirement.implementation &&
                    requirement.implementation->implementation_id ==
                        "air.flux2-klein.derive-schedule";
            }
            if (requirement.requirement.semantic_id ==
                "flux2-klein.denoiser") {
                denoiser_missing =
                    !requirement.resolved() &&
                    requirement.missing &&
                    requirement.missing->reason ==
                        air::MissingSemanticReason::not_registered;
            }
        }
        check(
            schedule_resolved && denoiser_missing,
            "snapshot distinguishes executable deterministic semantics from missing model component semantics");
    }
}

int main() {
    test_flux2_package_exposes_resolved_and_missing_semantics();
    test_unknown_requirement_is_structured_missing_semantic();
    test_trusted_test_extension_registers_without_core_edit();
    test_version_mismatch_is_not_guessed();
    test_package_resolution_is_data_only_and_complete();
    test_invalid_or_duplicate_data_is_rejected();

    if (failures != 0) {
        std::cerr << failures << " semantic extension contract test(s) failed\n";
        return 1;
    }

    std::cout
        << "Prompt 9 semantic extension/security contract passed\n";
    return 0;
}
