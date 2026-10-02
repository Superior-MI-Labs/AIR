#include "air/flux2_klein_semantics.hpp"

#include <concepts>
#include <cmath>
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
concept HasDevicePlacement = requires(T value) {
    value.device;
};

template <typename T>
concept HasStorageDtype = requires(T value) {
    value.dtype;
};

template <typename T>
concept HasTensorLayout = requires(T value) {
    value.layout;
};


void test_air_owned_latent_geometry_derivation_matches_oracle() {
    auto geometry =
        air::derive_flux2_klein_latent_geometry(1024U, 1024U, 1U);
    check(geometry.is_ok(), "AIR derives qualified FLUX.2 latent geometry");
    if (geometry) {
        check(
            geometry.value().batch == 1U &&
            geometry.value().channels == 128U &&
            geometry.value().height == 64U &&
            geometry.value().width == 64U,
            "AIR-owned latent geometry exactly matches Prompt 7 oracle");
    }

    check(
        !air::derive_flux2_klein_latent_geometry(
            1025U, 1024U, 1U).is_ok(),
        "adapter rejects dimensions that cannot satisfy qualified /16 geometry");
    check(
        !air::derive_flux2_klein_latent_geometry(
            1024U, 1024U, 0U).is_ok(),
        "adapter rejects zero active image instances");
}

void test_air_owned_schedule_derivation_matches_retained_source_formula() {
    auto schedule =
        air::derive_flux2_klein_schedule(1024U, 1024U, 4U);
    check(schedule.is_ok(), "AIR executes qualified FLUX.2 schedule derivation");
    if (!schedule) return;

    const std::array<double, 5> expected{
        1.000000000,
        0.967383988,
        0.908143923,
        0.767199964,
        0.000000000,
    };

    check(
        schedule.value().image_sequence_length == 4096U,
        "AIR derives qualified Flux2 image sequence length");
    check(
        std::abs(schedule.value().mu - 2.291179894115571) < 1.0e-12,
        "AIR derives retained scheduler shift mu");
    check(
        schedule.value().sigmas.size() == expected.size(),
        "AIR derives steps+1 sigma values");
    if (schedule.value().sigmas.size() == expected.size()) {
        bool matches = true;
        for (std::size_t i = 0; i < expected.size(); ++i) {
            matches =
                matches &&
                std::abs(schedule.value().sigmas[i] - expected[i]) <
                    1.0e-9;
        }
        check(
            matches,
            "AIR-owned scheduler reproduces retained Prompt 7 sigma path");
    }

    check(
        !air::derive_flux2_klein_schedule(
            1024U, 1024U, 0U).is_ok(),
        "AIR schedule executor rejects zero transitions");
}

void test_frozen_prompt7_oracle_semantics() {
    const auto contract = air::qualified_flux2_klein_oracle_semantics();

    check(
        air::validate_flux2_klein_oracle_semantics(contract).is_ok(),
        "frozen Prompt 7 FLUX.2 Klein semantic contract validates");
    check(
        contract.width == 1024U &&
        contract.height == 1024U &&
        contract.batch == 1U &&
        contract.iteration_count == 4U &&
        contract.seed == 432262096973490ULL,
        "frozen semantic configuration preserves qualified oracle values");
    check(
        contract.latent_geometry.batch == 1U &&
        contract.latent_geometry.channels == 128U &&
        contract.latent_geometry.height == 64U &&
        contract.latent_geometry.width == 64U,
        "qualified latent semantic geometry is preserved");
    check(
        contract.sigma_schedule ==
            std::array<double, 5>{
                1.000000000,
                0.967383988,
                0.908143923,
                0.767199964,
                0.000000000},
        "qualified five-value sigma path is preserved exactly");
}

void test_operation_boundary_matches_retained_prompt7_evidence() {
    const auto contract = air::qualified_flux2_klein_oracle_semantics();
    const std::array expected{
        air::Flux2KleinSemanticOperationKind::encode_conditioning,
        air::Flux2KleinSemanticOperationKind::derive_negative_conditioning,
        air::Flux2KleinSemanticOperationKind::initialize_latent,
        air::Flux2KleinSemanticOperationKind::derive_schedule,
        air::Flux2KleinSemanticOperationKind::realize_seeded_noise,
        air::Flux2KleinSemanticOperationKind::iterative_sample,
        air::Flux2KleinSemanticOperationKind::decode_latent,
    };

    check(
        contract.operation_sequence == expected,
        "semantic operation order matches retained Prompt 7 operation-boundary evidence");
    check(
        std::string(air::to_string(expected[3])) == "derive-schedule" &&
        std::string(air::to_string(expected[4])) == "realize-seeded-noise",
        "schedule-before-noise ordering is explicit rather than reconstructed from chat memory");
}

void test_semantic_identity_is_separate_from_storage_and_placement() {
    static_assert(!HasDevicePlacement<air::Flux2KleinSemanticContract>);
    static_assert(!HasStorageDtype<air::Flux2KleinSemanticContract>);
    static_assert(!HasTensorLayout<air::Flux2KleinSemanticContract>);

    const auto contract = air::qualified_flux2_klein_oracle_semantics();
    check(
        contract.values.positive_conditioning.kind ==
            air::Flux2KleinSemanticValueKind::conditioning &&
        contract.values.negative_conditioning.kind ==
            air::Flux2KleinSemanticValueKind::conditioning &&
        contract.values.positive_conditioning.id !=
            contract.values.negative_conditioning.id,
        "positive/negative conditioning share semantic kind but retain distinct identities");
    check(
        contract.values.initial_latent.kind ==
            air::Flux2KleinSemanticValueKind::latent &&
        contract.values.sampled_latent.kind ==
            air::Flux2KleinSemanticValueKind::latent &&
        contract.values.initial_latent.id !=
            contract.values.sampled_latent.id,
        "iterative latent state keeps stable before/after identities without encoding storage");
}

void test_frozen_oracle_contract_rejects_unqualified_drift() {
    auto changed_schedule = air::qualified_flux2_klein_oracle_semantics();
    changed_schedule.sigma_schedule[1] = 0.5;
    check(
        !air::validate_flux2_klein_oracle_semantics(
            changed_schedule).is_ok(),
        "unqualified sigma schedule drift is rejected");

    auto changed_seed = air::qualified_flux2_klein_oracle_semantics();
    ++changed_seed.seed;
    check(
        !air::validate_flux2_klein_oracle_semantics(changed_seed).is_ok(),
        "unqualified seed drift is rejected");

    auto duplicated_value = air::qualified_flux2_klein_oracle_semantics();
    duplicated_value.values.noise.id =
        duplicated_value.values.seed.id;
    check(
        !air::validate_flux2_klein_oracle_semantics(
            duplicated_value).is_ok(),
        "semantic value identities cannot alias accidentally");

    auto reordered = air::qualified_flux2_klein_oracle_semantics();
    const auto tmp = reordered.operation_sequence[3];
    reordered.operation_sequence[3] = reordered.operation_sequence[4];
    reordered.operation_sequence[4] = tmp;
    check(
        !air::validate_flux2_klein_oracle_semantics(reordered).is_ok(),
        "semantic operation order cannot drift from retained oracle evidence");
}

} // namespace

int main() {
    test_air_owned_latent_geometry_derivation_matches_oracle();
    test_air_owned_schedule_derivation_matches_retained_source_formula();
    test_frozen_prompt7_oracle_semantics();
    test_operation_boundary_matches_retained_prompt7_evidence();
    test_semantic_identity_is_separate_from_storage_and_placement();
    test_frozen_oracle_contract_rejects_unqualified_drift();

    if (failures != 0) {
        std::cerr << failures << " FLUX.2 semantic contract test(s) failed\n";
        return 1;
    }

    std::cout << "Prompt 8E FLUX.2 Klein semantic adapter contract passed\n";
    return 0;
}
