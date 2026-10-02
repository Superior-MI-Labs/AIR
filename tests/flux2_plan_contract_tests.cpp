#include "air/flux2_klein_plan.hpp"

#include <concepts>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

template <typename T>
concept HasEvictionDuration = requires(T value) {
    value.eviction_ms;
};

template <typename T>
concept HasLoadDuration = requires(T value) {
    value.load_ms;
};

template <typename T>
concept HasResidentBytes = requires(T value) {
    value.resident_bytes;
};

void test_component_plan_reuses_identified_resource_authority() {
    const auto semantics =
        air::qualified_flux2_klein_oracle_semantics();
    auto plan = air::make_flux2_klein_component_plan(
        semantics, "hardware-topology:test", "gpu0");

    check(plan.is_ok(), "qualified FLUX.2 component plan derives");
    if (!plan) return;

    check(
        air::validate_flux2_klein_component_plan(
            semantics, plan.value()).is_ok(),
        "derived FLUX.2 component plan validates");

    check(
        plan.value().phases.size() == 3U,
        "qualified plan has text-encoder, denoiser, and VAE phases");
    check(
        plan.value().phases[0].prepared_resources.front().resource_id ==
            air::flux2_klein_text_encoder_resource_id &&
        plan.value().phases[1].prepared_resources.front().resource_id ==
            air::flux2_klein_denoiser_resource_id &&
        plan.value().phases[2].prepared_resources.front().resource_id ==
            air::flux2_klein_vae_resource_id,
        "component phases use exact artifact-qualified prepared-resource identities");

    check(
        !plan.value().phases[0].prepared_resources.front().expected_device_bytes &&
        !plan.value().phases[1].prepared_resources.front().expected_device_bytes &&
        !plan.value().phases[2].prepared_resources.front().expected_device_bytes,
        "external staged-MB observations are not fabricated as AIR resident-byte requirements");
}

void test_only_denoiser_carries_qualified_iterative_work() {
    const auto semantics =
        air::qualified_flux2_klein_oracle_semantics();
    auto plan = air::make_flux2_klein_component_plan(
        semantics, "hardware-topology:test", "gpu0");
    if (!plan) {
        check(false, "iterative-work fixture derives");
        return;
    }

    check(
        !plan.value().phases[0].iterative_work &&
        plan.value().phases[1].iterative_work.has_value() &&
        !plan.value().phases[2].iterative_work,
        "only denoiser phase carries iterative physical work");
    if (plan.value().phases[1].iterative_work) {
        const auto& work = *plan.value().phases[1].iterative_work;
        const air::WorkloadPhysicalInvocation invocation{work};
        check(
            work.iteration_count == 4U &&
            work.active_instances == 1U &&
            air::physical_invocation_work_unit_kind(invocation) ==
                air::WorkUnitKind::iterations,
            "denoiser preserves four qualified iteration units");
    }
}

void test_component_order_and_semantic_handoff_are_explicit() {
    const auto semantics =
        air::qualified_flux2_klein_oracle_semantics();
    auto plan = air::make_flux2_klein_component_plan(
        semantics, "hardware-topology:test", "gpu0");
    if (!plan) {
        check(false, "semantic handoff fixture derives");
        return;
    }

    const auto& phases = plan.value().phases;
    check(
        phases[0].dependencies.empty() &&
        phases[1].dependencies == std::vector<std::uint32_t>{0U} &&
        phases[2].dependencies == std::vector<std::uint32_t>{1U},
        "component ordering is explicit and deterministic");

    check(
        phases[0].output_value_ids ==
            std::vector<std::string>{
                semantics.values.positive_conditioning.id,
                semantics.values.negative_conditioning.id} &&
        phases[1].output_value_ids ==
            std::vector<std::string>{
                semantics.values.sampled_latent.id} &&
        phases[2].output_value_ids ==
            std::vector<std::string>{
                semantics.values.decoded_image.id},
        "component phases reference stable adapter-owned output value identities");
}

void test_plan_does_not_fabricate_transition_or_residency_measurements() {
    static_assert(!HasEvictionDuration<air::Flux2KleinPhysicalPhase>);
    static_assert(!HasLoadDuration<air::Flux2KleinPhysicalPhase>);
    static_assert(!HasResidentBytes<air::Flux2KleinPhysicalPhase>);

    const auto semantics =
        air::qualified_flux2_klein_oracle_semantics();
    check(
        !air::make_flux2_klein_component_plan(
            semantics, "", "gpu0").is_ok(),
        "missing topology placement is rejected");
    check(
        !air::make_flux2_klein_component_plan(
            semantics, "hardware-topology:test", "").is_ok(),
        "missing hardware placement is rejected");
}

void test_plan_rejects_unqualified_semantic_or_resource_drift() {
    auto semantics =
        air::qualified_flux2_klein_oracle_semantics();
    auto plan = air::make_flux2_klein_component_plan(
        semantics, "hardware-topology:test", "gpu0");
    if (!plan) {
        check(false, "drift fixture derives");
        return;
    }

    auto wrong_resource = plan.value();
    wrong_resource.phases[1].prepared_resources.front().resource_id =
        "model/other/denoiser";
    check(
        !air::validate_flux2_klein_component_plan(
            semantics, wrong_resource).is_ok(),
        "component resource identity drift is rejected");

    auto invented_bytes = plan.value();
    invented_bytes.phases[0].prepared_resources.front().expected_device_bytes =
        3669U * 1024U * 1024U;
    check(
        !air::validate_flux2_klein_component_plan(
            semantics, invented_bytes).is_ok(),
        "unqualified conversion of staged MB to resident bytes is rejected");

    ++semantics.seed;
    check(
        !air::validate_flux2_klein_component_plan(
            semantics, plan.value()).is_ok(),
        "component plan cannot silently outlive its qualified semantic contract");
}

} // namespace

int main() {
    test_component_plan_reuses_identified_resource_authority();
    test_only_denoiser_carries_qualified_iterative_work();
    test_component_order_and_semantic_handoff_are_explicit();
    test_plan_does_not_fabricate_transition_or_residency_measurements();
    test_plan_rejects_unqualified_semantic_or_resource_drift();

    if (failures != 0) {
        std::cerr << failures << " FLUX.2 component plan test(s) failed\n";
        return 1;
    }

    std::cout << "Prompt 8F FLUX.2 component/resource plan contract passed\n";
    return 0;
}
