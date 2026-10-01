#include "air/workload.hpp"
#include "air/decision.hpp"

#include <iostream>
#include <string>
#include <type_traits>
#include <variant>

namespace {

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void test_existing_request_profile_is_preserved_as_autoregressive_contract() {
    static_assert(std::is_same_v<
        air::AutoregressiveRequestProfile,
        air::RequestProfile>);

    const air::RequestProfile original{128U, 32U, 3U};
    const air::ExecutionWorkloadProfile workload{original};

    check(
        air::execution_workload_kind(workload) ==
            air::ExecutionWorkloadKind::autoregressive_tokens,
        "existing RequestProfile must classify as autoregressive token work");
    check(
        air::work_unit_kind(workload) == air::WorkUnitKind::tokens,
        "autoregressive workload work units must be explicit tokens");
    check(
        air::active_workload_instances(workload) == 3U,
        "autoregressive active sequence count must survive workload wrapping");

    const auto* recovered =
        std::get_if<air::AutoregressiveRequestProfile>(&workload);
    check(recovered != nullptr, "wrapped autoregressive request must retain type");
    if (recovered != nullptr) {
        check(
            recovered->prompt_tokens == 128U &&
            recovered->max_output_tokens == 32U &&
            recovered->active_sequences == 3U,
            "workload boundary must preserve all qualified RequestProfile fields");
    }

    check(
        air::validate_execution_workload_profile(workload).is_ok(),
        "Prompt 8A must not add new validity restrictions to existing RequestProfile");
}

void test_iterative_profile_has_no_token_shape() {
    const air::ExecutionWorkloadProfile workload{
        air::IterativeRequestProfile{4U, 1U}};

    check(
        air::execution_workload_kind(workload) == air::ExecutionWorkloadKind::iterative_state,
        "iterative request must classify independently from token generation");
    check(
        air::work_unit_kind(workload) == air::WorkUnitKind::iterations,
        "iterative workload work units must be explicit iterations");
    check(
        air::active_workload_instances(workload) == 1U,
        "iterative active instance count must be inspectable");
    check(
        air::validate_execution_workload_profile(workload).is_ok(),
        "qualified four-iteration FLUX.2 request shape must validate");

    const auto* iterative =
        std::get_if<air::IterativeRequestProfile>(&workload);
    check(
        iterative != nullptr && iterative->iteration_count == 4U,
        "iterative profile must preserve the finite qualified iteration count");
}

void test_invalid_iterative_profiles_fail_without_guessing() {
    const air::ExecutionWorkloadProfile zero_iterations{
        air::IterativeRequestProfile{0U, 1U}};
    const air::ExecutionWorkloadProfile zero_instances{
        air::IterativeRequestProfile{4U, 0U}};

    check(
        !air::validate_execution_workload_profile(zero_iterations).is_ok(),
        "zero-iteration iterative workload must be rejected");
    check(
        !air::validate_execution_workload_profile(zero_instances).is_ok(),
        "zero-instance iterative workload must be rejected");
}

void test_service_semantics_and_execution_structure_are_separate_axes() {
    static_assert(!std::is_same_v<air::ExecutionWorkloadKind, air::WorkloadKind>);
    check(
        air::to_string(air::WorkloadKind::generation) == std::string("generation"),
        "existing service semantic workload vocabulary must remain unchanged");
    check(
        air::to_string(air::ExecutionWorkloadKind::autoregressive_tokens) ==
            std::string("autoregressive-tokens"),
        "execution workload structure must have a distinct inspection identity");
}

void test_work_measure_contract() {
    check(
        air::to_string(air::WorkUnitKind::bytes) == std::string("bytes"),
        "existing CUDA transfer byte counts require an explicit byte unit");
    check(
        air::validate_work_measure(std::nullopt, 0U).is_ok(),
        "zero-work administrative observations may omit a unit");
    check(
        !air::validate_work_measure(std::nullopt, 1U).is_ok(),
        "non-zero work may not remain untyped");
    check(
        air::validate_work_measure(air::WorkUnitKind::tokens, 8U).is_ok() &&
        air::validate_work_measure(air::WorkUnitKind::iterations, 4U).is_ok() &&
        air::validate_work_measure(air::WorkUnitKind::bytes, 64U).is_ok(),
        "qualified token, iteration, and byte work measures must validate");
}

void test_stable_inspection_names() {
    check(
        std::string(air::to_string(air::ExecutionWorkloadKind::autoregressive_tokens)) ==
            "autoregressive-tokens",
        "autoregressive workload inspection name must be stable");
    check(
        std::string(air::to_string(air::ExecutionWorkloadKind::iterative_state)) ==
            "iterative-state",
        "iterative workload inspection name must be stable");
    check(
        std::string(air::to_string(air::WorkUnitKind::tokens)) == "tokens",
        "token work-unit inspection name must be stable");
    check(
        std::string(air::to_string(air::WorkUnitKind::iterations)) ==
            "iterations",
        "iteration work-unit inspection name must be stable");
}

} // namespace

int main() {
    test_existing_request_profile_is_preserved_as_autoregressive_contract();
    test_iterative_profile_has_no_token_shape();
    test_invalid_iterative_profiles_fail_without_guessing();
    test_service_semantics_and_execution_structure_are_separate_axes();
    test_work_measure_contract();
    test_stable_inspection_names();

    if (failures != 0) {
        std::cerr << failures << " workload contract test(s) failed\n";
        return 1;
    }

    std::cout << "workload contract tests passed\n";
    return 0;
}
