#include "air/flux2_klein_extension.hpp"

#include <memory>
#include <utility>

namespace air {
namespace {

class StaticSemanticImplementation final : public SemanticImplementation {
public:
    explicit StaticSemanticImplementation(
        SemanticImplementationDescriptor descriptor)
        : descriptor_(std::move(descriptor)) {}

    [[nodiscard]] SemanticImplementationDescriptor descriptor()
        const override {
        return descriptor_;
    }

private:
    SemanticImplementationDescriptor descriptor_;
};

constexpr SemanticContractVersion v1{1U, 0U, 0U};

SemanticRequirement operation(std::string id) {
    return {
        SemanticRequirementKind::operation,
        std::move(id),
        v1,
    };
}

SemanticRequirement component(std::string id) {
    return {
        SemanticRequirementKind::component,
        std::move(id),
        v1,
    };
}

std::shared_ptr<const SemanticImplementation> implementation(
    SemanticRequirement requirement,
    std::string implementation_id) {
    return std::make_shared<const StaticSemanticImplementation>(
        SemanticImplementationDescriptor{
            std::move(requirement),
            std::move(implementation_id),
            v1,
        });
}

} // namespace

SemanticPackageDeclaration flux2_klein_semantic_package_declaration() {
    SemanticPackageDeclaration package;
    package.package_id = "flux2-klein.oracle-v1";
    package.requirements = {
        operation("flux2-klein.derive-latent-geometry"),
        operation("flux2-klein.encode-conditioning"),
        operation("flux2-klein.derive-negative-conditioning"),
        operation("flux2-klein.initialize-latent"),
        operation("flux2-klein.derive-schedule"),
        operation("flux2-klein.realize-seeded-noise"),
        operation("flux2-klein.iterative-sample"),
        operation("flux2-klein.decode-latent"),
        component("flux2-klein.text-encoder"),
        component("flux2-klein.denoiser"),
        component("flux2-klein.vae"),
    };
    return package;
}

Status register_flux2_klein_builtin_semantics(
    SemanticImplementationRegistry& registry) {
    const auto register_one =
        [&](SemanticRequirement requirement,
            std::string implementation_id) -> Status {
        return registry.register_trusted(
            implementation(
                std::move(requirement),
                std::move(implementation_id)),
            TrustedImplementationOrigin::built_in);
    };

    if (auto status = register_one(
            operation("flux2-klein.derive-latent-geometry"),
            "air.flux2-klein.derive-latent-geometry");
        !status) {
        return status;
    }

    if (auto status = register_one(
            operation("flux2-klein.derive-schedule"),
            "air.flux2-klein.derive-schedule");
        !status) {
        return status;
    }

    return Status::ok();
}

} // namespace air
