#include "air/resource.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
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

void test_equal_bytes_do_not_substitute_for_identity() {
    const std::vector<air::PreparedResourceRequirement> required{
        {"component/denoiser", 4096U},
    };
    const std::vector<air::PreparedResourceResidency> wrong{
        {"component/text-encoder",
         air::PreparedResourceResidencyState::resident,
         4096U},
    };

    const auto satisfied = air::prepared_resources_satisfy(required, wrong);
    check(satisfied.is_ok(), "valid identity mismatch must be evaluable");
    check(!satisfied.value(),
          "equal bytes from the wrong resource must never satisfy a requirement");
}

void test_residency_state_is_independent_from_nominal_identity() {
    const std::vector<air::PreparedResourceRequirement> required{
        {"component/vae", 160U},
    };
    const std::vector<air::PreparedResourceResidency> unknown{
        {"component/vae",
         air::PreparedResourceResidencyState::unknown,
         0U},
    };
    const std::vector<air::PreparedResourceResidency> nonresident{
        {"component/vae",
         air::PreparedResourceResidencyState::nonresident,
         0U},
    };
    check(!air::prepared_resources_satisfy(required, unknown).value(),
          "unknown residency must not be treated as hot");
    check(!air::prepared_resources_satisfy(required, nonresident).value(),
          "known nonresidency must not be treated as hot");
}

void test_multiple_independent_resources_and_derived_total() {
    const std::vector<air::PreparedResourceResidency> residencies{
        {"component/text-encoder",
         air::PreparedResourceResidencyState::resident,
         3669U},
        {"component/denoiser",
         air::PreparedResourceResidencyState::resident,
         3882U},
        {"component/vae",
         air::PreparedResourceResidencyState::resident,
         160U},
    };
    const auto bytes = air::resident_prepared_resource_bytes(residencies);
    check(bytes.is_ok(), "independent resource residency set must validate");
    check(bytes.value() == 7711U,
          "aggregate prepared bytes must be derived from identified residencies");
}

void test_invalid_duplicate_and_state_records_fail() {
    const std::vector<air::PreparedResourceRequirement> duplicate_requirements{
        {"x", std::nullopt},
        {"x", std::nullopt},
    };
    check(!air::validate_prepared_resource_requirements(
               duplicate_requirements).is_ok(),
          "duplicate required resource ids must fail");

    const std::vector<air::PreparedResourceResidency> impossible{
        {"x", air::PreparedResourceResidencyState::nonresident, 1U},
    };
    check(!air::validate_prepared_resource_residencies(impossible).is_ok(),
          "nonresident resource cannot report resident bytes");
}

void test_known_byte_requirement_is_a_floor_not_identity() {
    const std::vector<air::PreparedResourceRequirement> required{
        {"x", 100U},
    };
    const std::vector<air::PreparedResourceResidency> too_small{
        {"x", air::PreparedResourceResidencyState::resident, 99U},
    };
    const std::vector<air::PreparedResourceResidency> enough{
        {"x", air::PreparedResourceResidencyState::resident, 100U},
    };
    check(!air::prepared_resources_satisfy(required, too_small).value(),
          "matching id with insufficient measured bytes must remain cold");
    check(air::prepared_resources_satisfy(required, enough).value(),
          "matching id with sufficient measured bytes must be hot");
}

void test_qwen_plan_projects_prepared_tactic_identity() {
    air::ExecutionPlan baseline;
    baseline.backend = air::BackendKind::cuda;
    check(air::prepared_resource_requirements(baseline).empty(),
          "baseline CUDA plan must require no optional prepared resource");

    air::ExecutionPlan dense;
    dense.backend = air::BackendKind::cuda;
    dense.linear.prefill_block =
        air::QuantizedLinearExecutionKind::dense_f32_cublas;
    dense.linear.decode_block =
        air::QuantizedLinearExecutionKind::dense_f32_cublas;
    const auto dense_required = air::prepared_resource_requirements(dense);
    check(dense_required.size() == 1U,
          "repeated dense tactic must project one stable prepared resource");
    if (dense_required.size() == 1U) {
        check(dense_required[0].resource_id ==
                  air::cuda_dense_f32_cublas_resource_id,
              "dense tactic must map to stable CUDA prepared-resource id");
        check(!dense_required[0].expected_device_bytes.has_value(),
              "plan alone must not invent prepared resource bytes");
    }

    air::ExecutionPlan mixed = dense;
    mixed.linear.decode_block =
        air::QuantizedLinearExecutionKind::q5q8_dp4a_hybrid;
    const auto mixed_required = air::prepared_resource_requirements(mixed);
    check(mixed_required.size() == 2U,
          "distinct prepared tactics must remain distinct resource requirements");

    air::ExecutionPlan reference = dense;
    reference.backend = air::BackendKind::reference;
    check(air::prepared_resource_requirements(reference).empty(),
          "CUDA prepared-resource identity must not leak into reference backend");
}

} // namespace

int main() {
    test_equal_bytes_do_not_substitute_for_identity();
    test_residency_state_is_independent_from_nominal_identity();
    test_multiple_independent_resources_and_derived_total();
    test_invalid_duplicate_and_state_records_fail();
    test_known_byte_requirement_is_a_floor_not_identity();
    test_qwen_plan_projects_prepared_tactic_identity();

    if (failures != 0) {
        std::cerr << failures << " resource contract test(s) failed\n";
        return 1;
    }

    std::cout << "resource contract tests passed\n";
    return 0;
}
