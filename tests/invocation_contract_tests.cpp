#include "air/invocation.hpp"

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

template <typename T>
concept HasTokenParticipants = requires(T value) {
    value.participants;
};

template <typename T>
concept HasTokenOutputField = requires(T value) {
    value.output;
};

void test_qwen_member_preserves_existing_physical_invocation_type() {
    static_assert(std::is_same_v<
        air::AutoregressivePhysicalInvocation,
        air::PhysicalInvocation>);

    air::AutoregressivePhysicalInvocation qwen;
    qwen.kind = air::PhysicalInvocationKind::prefill_single;
    qwen.work_unit_kind = air::WorkUnitKind::tokens;
    qwen.participants.push_back(
        air::PhysicalInvocationParticipant{
            8U, air::PhysicalOutputMode::logits, 0U});
    qwen.topology_fingerprint = "hardware-topology:test";
    qwen.hardware_resource_id = "cpu0";

    const air::WorkloadPhysicalInvocation invocation{qwen};

    check(
        air::physical_invocation_workload_kind(invocation) ==
            air::ExecutionWorkloadKind::autoregressive_tokens,
        "Qwen physical invocation remains autoregressive token structure");
    check(
        air::physical_invocation_work_unit_kind(invocation) ==
            air::WorkUnitKind::tokens,
        "Qwen physical invocation reports token work units");
    check(
        air::validate_workload_physical_invocation_boundary(invocation).is_ok(),
        "qualified Qwen member validates at shared discrimination boundary");
}

void test_iterative_member_has_no_token_output_vocabulary() {
    static_assert(!HasTokenParticipants<air::IterativePhysicalInvocation>);
    static_assert(!HasTokenOutputField<air::IterativePhysicalInvocation>);

    const air::WorkloadPhysicalInvocation invocation{
        air::IterativePhysicalInvocation{
            4U,
            1U,
            "hardware-topology:test",
            "gpu0",
        }};

    check(
        air::physical_invocation_workload_kind(invocation) ==
            air::ExecutionWorkloadKind::iterative_state,
        "iterative physical invocation has independent workload structure");
    check(
        air::physical_invocation_work_unit_kind(invocation) ==
            air::WorkUnitKind::iterations,
        "iterative physical invocation fixes work units to iterations");
    check(
        air::validate_workload_physical_invocation_boundary(invocation).is_ok(),
        "qualified four-iteration physical invocation validates");
}

void test_iterative_boundary_rejects_unproven_or_invalid_structure() {
    const air::WorkloadPhysicalInvocation zero_iterations{
        air::IterativePhysicalInvocation{
            0U, 1U, "hardware-topology:test", "gpu0"}};
    const air::WorkloadPhysicalInvocation zero_instances{
        air::IterativePhysicalInvocation{
            4U, 0U, "hardware-topology:test", "gpu0"}};
    const air::WorkloadPhysicalInvocation no_topology{
        air::IterativePhysicalInvocation{
            4U, 1U, "", "gpu0"}};
    const air::WorkloadPhysicalInvocation no_resource{
        air::IterativePhysicalInvocation{
            4U, 1U, "hardware-topology:test", ""}};

    check(
        !air::validate_workload_physical_invocation_boundary(
            zero_iterations).is_ok(),
        "zero iterative work is rejected");
    check(
        !air::validate_workload_physical_invocation_boundary(
            zero_instances).is_ok(),
        "zero iterative concurrency is rejected");
    check(
        !air::validate_workload_physical_invocation_boundary(
            no_topology).is_ok(),
        "iterative invocation cannot invent missing topology");
    check(
        !air::validate_workload_physical_invocation_boundary(
            no_resource).is_ok(),
        "iterative invocation cannot invent missing placement resource");
}

void test_autoregressive_boundary_does_not_recreate_graph_legality() {
    air::AutoregressivePhysicalInvocation qwen;
    qwen.kind = air::PhysicalInvocationKind::prefill_single;
    qwen.work_unit_kind = air::WorkUnitKind::tokens;

    // Empty participants are illegal for ExecutionGraph R0, but that detailed
    // token-path rule remains owned by derive_execution_graph rather than being
    // duplicated here.
    const air::WorkloadPhysicalInvocation invocation{qwen};
    check(
        air::validate_workload_physical_invocation_boundary(invocation).is_ok(),
        "shared discrimination boundary does not become a second Qwen graph validator");

    qwen.work_unit_kind.reset();
    const air::WorkloadPhysicalInvocation untyped{qwen};
    check(
        !air::validate_workload_physical_invocation_boundary(untyped).is_ok(),
        "Stage 8C token-unit invariant remains enforced at shared boundary");
}

} // namespace

int main() {
    test_qwen_member_preserves_existing_physical_invocation_type();
    test_iterative_member_has_no_token_output_vocabulary();
    test_iterative_boundary_rejects_unproven_or_invalid_structure();
    test_autoregressive_boundary_does_not_recreate_graph_legality();

    if (failures != 0) {
        std::cerr << failures << " invocation contract test(s) failed\n";
        return 1;
    }

    std::cout << "Prompt 8D workload-scoped physical invocation contract passed\n";
    return 0;
}
