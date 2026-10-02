#include "air/execution.hpp"
#include "air/resource.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <type_traits>
#include <unordered_set>

namespace air {

const char* to_string(BackendKind backend) noexcept {
    switch (backend) {
    case BackendKind::reference: return "reference";
    case BackendKind::cuda: return "cuda";
    }
    return "unknown";
}

Result<BackendKind> backend_kind_from_string(std::string_view value) {
    if (value == "reference") return BackendKind::reference;
    if (value == "cuda") return BackendKind::cuda;
    return Status::data_error("unknown backend kind: " + std::string(value));
}

const char* to_string(PrefillExecutionKind kind) noexcept {
    switch (kind) {
    case PrefillExecutionKind::serial: return "serial";
    case PrefillExecutionKind::native_batch: return "native-batch";
    }
    return "unknown";
}

const char* to_string(KvStorageKind kind) noexcept {
    switch (kind) {
    case KvStorageKind::contiguous: return "contiguous";
    case KvStorageKind::paged: return "paged";
    }
    return "unknown";
}

const char* to_string(QuantizedLinearExecutionKind kind) noexcept {
    switch (kind) {
    case QuantizedLinearExecutionKind::baseline: return "baseline";
    case QuantizedLinearExecutionKind::batch_reuse4: return "batch-reuse4";
    case QuantizedLinearExecutionKind::batch_reuse8: return "batch-reuse8";
    case QuantizedLinearExecutionKind::q5q8_dp4a_hybrid: return "q5q8-dp4a-hybrid";
    case QuantizedLinearExecutionKind::dense_f32_cublas: return "dense-f32-cublas";
    }
    return "unknown";
}

Result<QuantizedLinearExecutionKind> quantized_linear_execution_kind_from_string(
    std::string_view value) {
    if (value == "baseline") return QuantizedLinearExecutionKind::baseline;
    if (value == "batch-reuse4" || value == "reuse4") return QuantizedLinearExecutionKind::batch_reuse4;
    if (value == "batch-reuse8" || value == "reuse8") {
        return QuantizedLinearExecutionKind::batch_reuse8;
    }
    if (value == "dense-f32-cublas" || value == "f32-cublas" || value == "dense-f32") {
        return QuantizedLinearExecutionKind::dense_f32_cublas;
    }
    return Status::data_error("unknown quantized linear execution kind: " + std::string(value));
}


const char* to_string(AttentionExecutionKind kind) noexcept {
    switch (kind) {
    case AttentionExecutionKind::baseline: return "baseline";
    case AttentionExecutionKind::online_softmax: return "online-softmax";
    }
    return "unknown";
}

Result<AttentionExecutionKind> attention_execution_kind_from_string(std::string_view value) {
    if (value == "baseline") return AttentionExecutionKind::baseline;
    if (value == "online-softmax" || value == "online") {
        return AttentionExecutionKind::online_softmax;
    }
    return Status::data_error("unknown attention execution kind: " + std::string(value));
}

const char* to_string(QualifiedOperationSite site) noexcept {
    switch (site) {
    case QualifiedOperationSite::prefill_transformer_block_linear:
        return "prefill-transformer-block-linear";
    case QualifiedOperationSite::decode_transformer_block_linear:
        return "decode-transformer-block-linear";
    case QualifiedOperationSite::decode_output_projection:
        return "decode-output-projection";
    case QualifiedOperationSite::prefill_attention:
        return "prefill-attention";
    case QualifiedOperationSite::decode_attention:
        return "decode-attention";
    }
    return "unknown";
}

const char* to_string(OperationImplementationFamily family) noexcept {
    switch (family) {
    case OperationImplementationFamily::linear: return "linear";
    case OperationImplementationFamily::attention: return "attention";
    }
    return "unknown";
}

Result<OperationImplementationFamily> implementation_family(
    QualifiedOperationSite site) {
    switch (site) {
    case QualifiedOperationSite::prefill_transformer_block_linear:
    case QualifiedOperationSite::decode_transformer_block_linear:
    case QualifiedOperationSite::decode_output_projection:
        return OperationImplementationFamily::linear;
    case QualifiedOperationSite::prefill_attention:
    case QualifiedOperationSite::decode_attention:
        return OperationImplementationFamily::attention;
    }
    return Status::invalid_argument("unknown qualified operation site");
}

Result<std::span<const QuantizedLinearExecutionKind>> linear_implementations(
    const BackendCapabilities& capabilities,
    QualifiedOperationSite site) {
    switch (site) {
    case QualifiedOperationSite::prefill_transformer_block_linear:
        return std::span<const QuantizedLinearExecutionKind>(
            capabilities.prefill_block_quantized_linear);
    case QualifiedOperationSite::decode_transformer_block_linear:
        return std::span<const QuantizedLinearExecutionKind>(
            capabilities.decode_block_quantized_linear);
    case QualifiedOperationSite::decode_output_projection:
        return std::span<const QuantizedLinearExecutionKind>(
            capabilities.decode_output_quantized_linear);
    case QualifiedOperationSite::prefill_attention:
    case QualifiedOperationSite::decode_attention:
        return Status::invalid_argument(
            "qualified operation site does not use a linear implementation family");
    }
    return Status::invalid_argument("unknown qualified operation site");
}

Result<std::span<const AttentionExecutionKind>> attention_implementations(
    const BackendCapabilities& capabilities,
    QualifiedOperationSite site) {
    switch (site) {
    case QualifiedOperationSite::prefill_attention:
        return std::span<const AttentionExecutionKind>(
            capabilities.prefill_attention);
    case QualifiedOperationSite::decode_attention:
        return std::span<const AttentionExecutionKind>(
            capabilities.decode_attention);
    case QualifiedOperationSite::prefill_transformer_block_linear:
    case QualifiedOperationSite::decode_transformer_block_linear:
    case QualifiedOperationSite::decode_output_projection:
        return Status::invalid_argument(
            "qualified operation site does not use an attention implementation family");
    }
    return Status::invalid_argument("unknown qualified operation site");
}

Status validate_execution_plan(const ExecutionPlan& plan,
                               const BackendCapabilities& capabilities) {
    if (plan.backend != capabilities.backend) {
        return Status::invalid_argument("execution plan backend does not match prepared backend capability");
    }
    if (plan.strategy_id.empty()) {
        return Status::invalid_argument("execution plan strategy id must not be empty");
    }
    if (plan.scheduling.prefill_quantum_tokens == 0U) {
        return Status::invalid_argument("execution plan prefill quantum must be non-zero");
    }
    if (capabilities.kv_storage == KvStorageKind::paged) {
        if (!plan.kv.page_tokens || *plan.kv.page_tokens == 0U) {
            return Status::invalid_argument("paged backend requires a non-zero physical KV page size");
        }
    } else if (plan.kv.page_tokens) {
        return Status::invalid_argument("contiguous backend cannot accept a physical KV page size");
    }
    if (capabilities.max_prefill_batch_width == 0U || capabilities.max_decode_batch_width == 0U) {
        return Status::invalid_state("backend capabilities must expose non-zero batch widths");
    }
    if (capabilities.exact_prefix_reuse && !capabilities.sequence_checkpointing) {
        return Status::invalid_state("exact prefix reuse requires sequence checkpointing capability");
    }
    const auto supports_linear = [&](QualifiedOperationSite site,
                                     QuantizedLinearExecutionKind value) {
        auto implementations = linear_implementations(capabilities, site);
        return implementations &&
            std::find(
                implementations.value().begin(),
                implementations.value().end(),
                value) != implementations.value().end();
    };
    if (!supports_linear(
            QualifiedOperationSite::prefill_transformer_block_linear,
            plan.linear.prefill_block)) {
        return Status::unsupported(
            "execution plan requests unsupported prefill block-linear tactic");
    }
    if (!supports_linear(
            QualifiedOperationSite::decode_transformer_block_linear,
            plan.linear.decode_block)) {
        return Status::unsupported(
            "execution plan requests unsupported decode block-linear tactic");
    }
    if (!supports_linear(
            QualifiedOperationSite::decode_output_projection,
            plan.linear.decode_output)) {
        return Status::unsupported(
            "execution plan requests unsupported decode output-projection tactic");
    }

    const auto supports_attention = [&](QualifiedOperationSite site,
                                        AttentionExecutionKind value) {
        auto implementations = attention_implementations(capabilities, site);
        return implementations &&
            std::find(
                implementations.value().begin(),
                implementations.value().end(),
                value) != implementations.value().end();
    };
    if (!supports_attention(
            QualifiedOperationSite::prefill_attention,
            plan.attention.prefill)) {
        return Status::unsupported(
            "execution plan requests unsupported prefill attention tactic");
    }
    if (!supports_attention(
            QualifiedOperationSite::decode_attention,
            plan.attention.decode)) {
        return Status::unsupported(
            "execution plan requests unsupported decode attention tactic");
    }
    return Status::ok();
}

const char* to_string(ExecutionGraphBindingKind kind) noexcept {
    switch (kind) {
    case ExecutionGraphBindingKind::descriptive:
        return "descriptive";
    case ExecutionGraphBindingKind::air_executable:
        return "air-executable";
    }
    return "unknown";
}

const char* to_string(ExecutionGraphNodeKind kind) noexcept {
    switch (kind) {
    case ExecutionGraphNodeKind::compute_region: return "compute-region";
    case ExecutionGraphNodeKind::transfer_region: return "transfer-region";
    case ExecutionGraphNodeKind::synchronization_region:
        return "synchronization-region";
    }
    return "unknown";
}

const char* to_string(ExecutionComputeRegionKind kind) noexcept {
    switch (kind) {
    case ExecutionComputeRegionKind::model: return "model";
    case ExecutionComputeRegionKind::component: return "component";
    case ExecutionComputeRegionKind::device_greedy_selection:
        return "device-greedy-selection";
    case ExecutionComputeRegionKind::target_logprob_reduction:
        return "target-logprob-reduction";
    }
    return "unknown";
}

const char* to_string(ExecutionTransferDirection direction) noexcept {
    switch (direction) {
    case ExecutionTransferDirection::host_to_device: return "host-to-device";
    case ExecutionTransferDirection::device_to_host: return "device-to-host";
    }
    return "unknown";
}

const char* to_string(ExecutionPayloadKind payload) noexcept {
    switch (payload) {
    case ExecutionPayloadKind::input_tokens: return "input-tokens";
    case ExecutionPayloadKind::full_logits: return "full-logits";
    case ExecutionPayloadKind::greedy_result: return "greedy-result";
    case ExecutionPayloadKind::target_tokens: return "target-tokens";
    case ExecutionPayloadKind::target_logprob_results:
        return "target-logprob-results";
    }
    return "unknown";
}

const char* to_string(ExecutionSynchronizationKind kind) noexcept {
    switch (kind) {
    case ExecutionSynchronizationKind::backend_stream_wait:
        return "backend-stream-wait";
    }
    return "unknown";
}

namespace {

[[nodiscard]] bool valid_invocation_kind(PhysicalInvocationKind kind) noexcept {
    switch (kind) {
    case PhysicalInvocationKind::prefill_single:
    case PhysicalInvocationKind::prefill_native_batch:
    case PhysicalInvocationKind::decode_single:
    case PhysicalInvocationKind::decode_native_greedy_batch:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_output_mode(PhysicalOutputMode mode) noexcept {
    switch (mode) {
    case PhysicalOutputMode::discard:
    case PhysicalOutputMode::logits:
    case PhysicalOutputMode::greedy:
    case PhysicalOutputMode::target_logprobs:
        return true;
    }
    return false;
}

[[nodiscard]] ExecutionImplementationBinding linear_binding(
    QualifiedOperationSite site,
    QuantizedLinearExecutionKind implementation) {
    ExecutionImplementationBinding binding;
    binding.site = site;
    binding.linear = implementation;
    return binding;
}

[[nodiscard]] ExecutionImplementationBinding attention_binding(
    QualifiedOperationSite site,
    AttentionExecutionKind implementation) {
    ExecutionImplementationBinding binding;
    binding.site = site;
    binding.attention = implementation;
    return binding;
}

[[nodiscard]] std::vector<ExecutionImplementationBinding> model_bindings(
    const ExecutionPlan& plan,
    PhysicalInvocationKind kind) {
    switch (kind) {
    case PhysicalInvocationKind::prefill_single:
    case PhysicalInvocationKind::prefill_native_batch:
        return {
            linear_binding(
                QualifiedOperationSite::prefill_transformer_block_linear,
                plan.linear.prefill_block),
            attention_binding(
                QualifiedOperationSite::prefill_attention,
                plan.attention.prefill),
        };
    case PhysicalInvocationKind::decode_single:
    case PhysicalInvocationKind::decode_native_greedy_batch:
        return {
            linear_binding(
                QualifiedOperationSite::decode_transformer_block_linear,
                plan.linear.decode_block),
            linear_binding(
                QualifiedOperationSite::decode_output_projection,
                plan.linear.decode_output),
            attention_binding(
                QualifiedOperationSite::decode_attention,
                plan.attention.decode),
        };
    }
    return {};
}

[[nodiscard]] std::string canonical_graph_body(const ExecutionGraph& graph) {
    std::ostringstream out;
    out << "schema=" << graph.schema_version() << '\n';
    out << "backend=" << to_string(graph.backend()) << '\n';
    out << "binding=" << to_string(graph.binding()) << '\n';
    out << "workload-kind=" << to_string(graph.workload_kind()) << '\n';

    std::visit(
        [&](const auto& invocation) {
            using T = std::decay_t<decltype(invocation)>;
            if constexpr (std::is_same_v<T, AutoregressivePhysicalInvocation>) {
                out << "invocation=" << to_string(invocation.kind) << '\n';
                out << "work-unit-kind=";
                if (invocation.work_unit_kind) {
                    out << to_string(*invocation.work_unit_kind);
                } else {
                    out << "none";
                }
                out << '\n';
                out << "topology=" << invocation.topology_fingerprint.size()
                    << ':' << invocation.topology_fingerprint << '\n';
                out << "resource=" << invocation.hardware_resource_id.size()
                    << ':' << invocation.hardware_resource_id << '\n';
                out << "participants=" << invocation.participants.size() << '\n';
                for (std::size_t i = 0; i < invocation.participants.size(); ++i) {
                    const auto& participant = invocation.participants[i];
                    out << "participant=" << i
                        << "|work=" << participant.work_units
                        << "|output=" << to_string(participant.output)
                        << "|targets=" << participant.target_count << '\n';
                }
            } else {
                out << "invocation=iterative-state\n";
                out << "work-unit-kind=iterations\n";
                out << "topology=" << invocation.topology_fingerprint.size()
                    << ':' << invocation.topology_fingerprint << '\n';
                out << "resource=" << invocation.hardware_resource_id.size()
                    << ':' << invocation.hardware_resource_id << '\n';
                out << "iterations=" << invocation.iteration_count << '\n';
                out << "active-instances=" << invocation.active_instances << '\n';
                out << "participants=0\n";
            }
        },
        graph.workload_invocation());
    out << "autoregressive-state=" << (graph.autoregressive_state() ? "present" : "none") << '\n';
    out << "state-storage=";
    if (graph.autoregressive_state()) {
        out << to_string(graph.autoregressive_state()->storage);
    } else {
        out << "none";
    }
    out << '\n';
    out << "state-page-tokens=";
    if (graph.autoregressive_state() && graph.autoregressive_state()->page_tokens) {
        out << *graph.autoregressive_state()->page_tokens;
    } else {
        out << "none";
    }
    out << '\n';

    out << "nodes=" << graph.nodes().size() << '\n';
    for (const auto& node : graph.nodes()) {
        out << "node=" << node.id
            << "|kind=" << to_string(node.kind)
            << "|resource=" << node.hardware_resource_id.size()
            << ':' << node.hardware_resource_id
            << "|participants=" << node.participant_count
            << "|work-unit=";
        if (node.work_unit_kind) out << to_string(*node.work_unit_kind);
        else out << "none";
        out << "|work=" << node.work_units
            << "|items=" << node.item_count
            << "|compute=";
        if (node.compute) out << to_string(*node.compute);
        else out << "none";
        out << "|direction=";
        if (node.transfer_direction) out << to_string(*node.transfer_direction);
        else out << "none";
        out << "|payload=";
        if (node.payload) out << to_string(*node.payload);
        else out << "none";
        out << "|sync=";
        if (node.synchronization) out << to_string(*node.synchronization);
        else out << "none";

        out << "|deps=";
        for (std::size_t i = 0; i < node.dependencies.size(); ++i) {
            if (i != 0U) out << ',';
            out << node.dependencies[i];
        }

        out << "|prepared=";
        for (std::size_t i = 0; i < node.prepared_resource_ids.size(); ++i) {
            if (i != 0U) out << ',';
            out << node.prepared_resource_ids[i].size() << ':'
                << node.prepared_resource_ids[i];
        }

        out << "|inputs=";
        for (std::size_t i = 0; i < node.input_value_ids.size(); ++i) {
            if (i != 0U) out << ',';
            out << node.input_value_ids[i].size() << ':' << node.input_value_ids[i];
        }

        out << "|outputs=";
        for (std::size_t i = 0; i < node.output_value_ids.size(); ++i) {
            if (i != 0U) out << ',';
            out << node.output_value_ids[i].size() << ':' << node.output_value_ids[i];
        }

        out << "|impl=";
        for (std::size_t i = 0; i < node.implementations.size(); ++i) {
            if (i != 0U) out << ',';
            const auto& implementation = node.implementations[i];
            out << to_string(implementation.site) << ':';
            if (implementation.linear) {
                out << "linear=" << to_string(*implementation.linear);
            } else if (implementation.attention) {
                out << "attention=" << to_string(*implementation.attention);
            } else {
                out << "none";
            }
        }
        out << '\n';
    }
    return out.str();
}

[[nodiscard]] std::string graph_identity(std::string_view canonical) {
    std::uint64_t value = 14695981039346656037ULL;
    for (const unsigned char byte : canonical) {
        value ^= static_cast<std::uint64_t>(byte);
        value *= 1099511628211ULL;
    }
    std::ostringstream out;
    out << "execution-graph:r1:"
        << std::hex << std::setfill('0') << std::setw(16) << value;
    return out.str();
}

} // namespace

Result<ExecutionGraph> finalize_execution_graph(
    BackendKind backend,
    ExecutionGraphBindingKind binding,
    WorkloadPhysicalInvocation invocation,
    std::optional<AutoregressiveExecutionState> autoregressive_state,
    std::vector<ExecutionGraphNode> nodes) {
    const auto invocation_status =
        validate_workload_physical_invocation_boundary(invocation);
    if (!invocation_status) return invocation_status;

    const auto workload_kind =
        physical_invocation_workload_kind(invocation);
    if (workload_kind == ExecutionWorkloadKind::autoregressive_tokens) {
        if (!autoregressive_state) {
            return Status::invalid_argument(
                "autoregressive execution graph requires autoregressive state");
        }
    } else if (autoregressive_state) {
        return Status::invalid_argument(
            "iterative execution graph must not carry KV/autoregressive state");
    }

    if (nodes.empty()) {
        return Status::invalid_argument(
            "execution graph requires at least one physical node");
    }

    const auto invocation_unit =
        physical_invocation_work_unit_kind(invocation);
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        if (node.id != i) {
            return Status::invalid_argument(
                "execution graph node ids must be contiguous and ordered");
        }
        if (node.hardware_resource_id.empty()) {
            return Status::invalid_argument(
                "execution graph node requires hardware resource identity");
        }
        const auto work_status =
            validate_work_measure(node.work_unit_kind, node.work_units);
        if (!work_status) return work_status;
        if (node.work_unit_kind &&
            *node.work_unit_kind != invocation_unit) {
            return Status::invalid_argument(
                "execution graph node workload unit differs from invocation");
        }
        for (const auto dependency : node.dependencies) {
            if (dependency >= node.id) {
                return Status::invalid_argument(
                    "execution graph dependency must reference an earlier node");
            }
        }

        const bool compute = node.compute.has_value();
        const bool transfer = node.transfer_direction.has_value();
        const bool synchronization = node.synchronization.has_value();
        switch (node.kind) {
        case ExecutionGraphNodeKind::compute_region:
            if (!compute || transfer || synchronization) {
                return Status::invalid_argument(
                    "compute-region graph node has conflicting physical kind");
            }
            break;
        case ExecutionGraphNodeKind::transfer_region:
            if (compute || !transfer || synchronization) {
                return Status::invalid_argument(
                    "transfer-region graph node has conflicting physical kind");
            }
            break;
        case ExecutionGraphNodeKind::synchronization_region:
            if (compute || transfer || !synchronization) {
                return Status::invalid_argument(
                    "synchronization graph node has conflicting physical kind");
            }
            break;
        }

        const auto validate_ids = [](const auto& ids) -> Status {
            std::unordered_set<std::string> seen;
            for (const auto& id : ids) {
                if (id.empty()) {
                    return Status::invalid_argument(
                        "execution graph opaque identity must not be empty");
                }
                if (!seen.insert(id).second) {
                    return Status::invalid_argument(
                        "execution graph opaque identities must be unique within a field");
                }
            }
            return Status::ok();
        };
        if (const auto s = validate_ids(node.prepared_resource_ids); !s) return s;
        if (const auto s = validate_ids(node.input_value_ids); !s) return s;
        if (const auto s = validate_ids(node.output_value_ids); !s) return s;
    }

    ExecutionGraph structural(
        "",
        backend,
        binding,
        std::move(invocation),
        std::move(autoregressive_state),
        std::move(nodes));
    const auto canonical = canonical_graph_body(structural);
    return ExecutionGraph(
        graph_identity(canonical),
        structural.backend(),
        structural.binding(),
        structural.workload_invocation(),
        structural.autoregressive_state(),
        std::vector<ExecutionGraphNode>(
            structural.nodes().begin(), structural.nodes().end()));
}

Result<ExecutionGraph> derive_execution_graph(
    const ExecutionPlan& plan,
    const BackendCapabilities& capabilities,
    PhysicalInvocation invocation) {
    const auto plan_status = validate_execution_plan(plan, capabilities);
    if (!plan_status) return plan_status;

    if (!valid_invocation_kind(invocation.kind)) {
        return Status::invalid_argument("physical invocation kind is unknown");
    }
    const auto work_measure =
        validate_work_measure(invocation.work_unit_kind, 1U);
    if (!work_measure) return work_measure;
    if (*invocation.work_unit_kind != WorkUnitKind::tokens) {
        return Status::unsupported(
            "current prefill/decode physical invocation vocabulary requires token work units");
    }
    if (invocation.topology_fingerprint.empty()) {
        return Status::invalid_argument(
            "physical invocation requires an existing hardware topology fingerprint");
    }
    if (invocation.hardware_resource_id.empty()) {
        return Status::invalid_argument(
            "physical invocation requires an existing hardware resource identity");
    }
    if (invocation.participants.empty()) {
        return Status::invalid_argument(
            "physical invocation requires at least one participant");
    }
    if (invocation.participants.size() >
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return Status::invalid_argument("physical invocation participant count is too large");
    }

    bool any_greedy = false;
    bool any_discard = false;
    bool any_logits = false;
    bool any_target = false;
    std::uint64_t checked_total_work = 0U;
    std::uint64_t checked_total_targets = 0U;
    for (const auto& participant : invocation.participants) {
        if (participant.work_units == 0U) {
            return Status::invalid_argument(
                "physical invocation participant work must be non-zero");
        }
        if (!valid_output_mode(participant.output)) {
            return Status::invalid_argument("physical invocation output mode is unknown");
        }
        if (participant.output == PhysicalOutputMode::target_logprobs) {
            if (participant.target_count == 0U) {
                return Status::invalid_argument(
                    "target-logprob physical output requires at least one target");
            }
            if (participant.target_count >
                std::numeric_limits<std::uint64_t>::max() - checked_total_targets) {
                return Status::invalid_argument(
                    "physical invocation target-count total overflows");
            }
            checked_total_targets += participant.target_count;
            any_target = true;
        } else if (participant.target_count != 0U) {
            return Status::invalid_argument(
                "target count is only valid for target-logprob physical output");
        }
        any_greedy = any_greedy || participant.output == PhysicalOutputMode::greedy;
        any_discard = any_discard || participant.output == PhysicalOutputMode::discard;
        any_logits = any_logits || participant.output == PhysicalOutputMode::logits;

        if (participant.work_units >
            std::numeric_limits<std::uint64_t>::max() - checked_total_work) {
            return Status::invalid_argument("physical invocation work-unit total overflows");
        }
        checked_total_work += participant.work_units;
    }

    const auto participant_count =
        static_cast<std::uint32_t>(invocation.participants.size());
    const bool is_prefill =
        invocation.kind == PhysicalInvocationKind::prefill_single ||
        invocation.kind == PhysicalInvocationKind::prefill_native_batch;
    switch (invocation.kind) {
    case PhysicalInvocationKind::prefill_single:
        if (participant_count != 1U) {
            return Status::invalid_argument(
                "single prefill physical invocation requires one participant");
        }
        break;
    case PhysicalInvocationKind::prefill_native_batch:
        if (participant_count < 2U) {
            return Status::invalid_argument(
                "native prefill batch requires at least two participants");
        }
        if (capabilities.prefill_execution != PrefillExecutionKind::native_batch) {
            return Status::unsupported(
                "prepared backend does not expose native prefill batching");
        }
        if (participant_count > capabilities.max_prefill_batch_width) {
            return Status::unsupported(
                "native prefill batch exceeds prepared backend width");
        }
        if (any_logits || any_target) {
            return Status::unsupported(
                "current native prefill batch supports only discard/greedy outputs");
        }
        break;
    case PhysicalInvocationKind::decode_single:
        if (participant_count != 1U) {
            return Status::invalid_argument(
                "single decode physical invocation requires one participant");
        }
        if (any_discard) {
            return Status::unsupported(
                "current decode physical invocation does not support discard output");
        }
        break;
    case PhysicalInvocationKind::decode_native_greedy_batch:
        if (participant_count < 2U) {
            return Status::invalid_argument(
                "native greedy decode batch requires at least two participants");
        }
        if (participant_count > capabilities.max_decode_batch_width) {
            return Status::unsupported(
                "native greedy decode batch exceeds prepared backend width");
        }
        if (!any_greedy || any_discard || any_logits || any_target) {
            return Status::invalid_argument(
                "native greedy decode batch requires greedy output for every participant");
        }
        for (const auto& participant : invocation.participants) {
            if (participant.output != PhysicalOutputMode::greedy) {
                return Status::invalid_argument(
                    "native greedy decode batch contains a non-greedy participant");
            }
        }
        break;
    }

    if (any_greedy && !capabilities.device_greedy_selection) {
        return Status::unsupported(
            "prepared backend does not expose device greedy selection");
    }

    std::vector<ExecutionGraphNode> nodes;
    nodes.reserve(8U);
    std::optional<std::uint32_t> tail;

    const auto append_node = [&](ExecutionGraphNode node) {
        node.id = static_cast<std::uint32_t>(nodes.size());
        if (tail) node.dependencies.push_back(*tail);
        nodes.push_back(std::move(node));
        tail = nodes.back().id;
        return *tail;
    };

    const auto make_node = [&](ExecutionGraphNodeKind kind) {
        ExecutionGraphNode node;
        node.kind = kind;
        node.hardware_resource_id = invocation.hardware_resource_id;
        node.participant_count = participant_count;
        node.work_unit_kind = WorkUnitKind::tokens;
        node.work_units = checked_total_work;
        return node;
    };

    // CUDA native token-matrix paths explicitly enqueue input token IDs.
    // Scalar decode passes the token as scalar launch data and has no equivalent
    // H2D token-array enqueue boundary.
    if (capabilities.backend == BackendKind::cuda &&
        (is_prefill || invocation.kind ==
            PhysicalInvocationKind::decode_native_greedy_batch)) {
        auto input = make_node(ExecutionGraphNodeKind::transfer_region);
        input.transfer_direction = ExecutionTransferDirection::host_to_device;
        input.payload = ExecutionPayloadKind::input_tokens;
        append_node(std::move(input));
    }

    auto model = make_node(ExecutionGraphNodeKind::compute_region);
    model.compute = ExecutionComputeRegionKind::model;
    model.implementations = model_bindings(plan, invocation.kind);
    for (const auto& requirement : prepared_resource_requirements(plan)) {
        model.prepared_resource_ids.push_back(requirement.resource_id);
    }
    append_node(std::move(model));

    if (capabilities.backend == BackendKind::cuda) {
        if (any_target) {
            auto targets = make_node(ExecutionGraphNodeKind::transfer_region);
            targets.transfer_direction = ExecutionTransferDirection::host_to_device;
            targets.payload = ExecutionPayloadKind::target_tokens;
            targets.work_unit_kind.reset();
            targets.work_units = 0U;
            targets.item_count = checked_total_targets;
            append_node(std::move(targets));

            auto reduction = make_node(ExecutionGraphNodeKind::compute_region);
            reduction.compute = ExecutionComputeRegionKind::target_logprob_reduction;
            reduction.work_unit_kind.reset();
            reduction.work_units = 0U;
            reduction.item_count = checked_total_targets;
            append_node(std::move(reduction));

            auto results = make_node(ExecutionGraphNodeKind::transfer_region);
            results.transfer_direction = ExecutionTransferDirection::device_to_host;
            results.payload = ExecutionPayloadKind::target_logprob_results;
            results.work_unit_kind.reset();
            results.work_units = 0U;
            results.item_count = checked_total_targets;
            append_node(std::move(results));

            auto wait = make_node(ExecutionGraphNodeKind::synchronization_region);
            wait.synchronization =
                ExecutionSynchronizationKind::backend_stream_wait;
            wait.work_unit_kind.reset();
            wait.work_units = 0U;
            append_node(std::move(wait));
        } else if (any_greedy) {
            std::uint32_t greedy_participants = 0U;
            for (const auto& participant : invocation.participants) {
                if (participant.output == PhysicalOutputMode::greedy) {
                    ++greedy_participants;
                }
            }

            auto selection = make_node(ExecutionGraphNodeKind::compute_region);
            selection.compute = ExecutionComputeRegionKind::device_greedy_selection;
            selection.participant_count = greedy_participants;
            selection.work_unit_kind.reset();
            selection.work_units = 0U;
            selection.item_count = greedy_participants;
            append_node(std::move(selection));

            auto result = make_node(ExecutionGraphNodeKind::transfer_region);
            result.transfer_direction = ExecutionTransferDirection::device_to_host;
            result.payload = ExecutionPayloadKind::greedy_result;
            result.participant_count = greedy_participants;
            result.work_unit_kind.reset();
            result.work_units = 0U;
            result.item_count = greedy_participants;
            append_node(std::move(result));

            auto wait = make_node(ExecutionGraphNodeKind::synchronization_region);
            wait.synchronization =
                ExecutionSynchronizationKind::backend_stream_wait;
            wait.participant_count = greedy_participants;
            wait.work_unit_kind.reset();
            wait.work_units = 0U;
            append_node(std::move(wait));
        } else if (any_logits) {
            auto logits = make_node(ExecutionGraphNodeKind::transfer_region);
            logits.transfer_direction = ExecutionTransferDirection::device_to_host;
            logits.payload = ExecutionPayloadKind::full_logits;
            // Exact bytes depend on model vocabulary geometry, which is not
            // owned by this R0 invocation descriptor. Do not invent a count.
            logits.work_unit_kind.reset();
            logits.work_units = 0U;
            logits.item_count = participant_count;
            append_node(std::move(logits));

            auto wait = make_node(ExecutionGraphNodeKind::synchronization_region);
            wait.synchronization =
                ExecutionSynchronizationKind::backend_stream_wait;
            wait.work_unit_kind.reset();
            wait.work_units = 0U;
            append_node(std::move(wait));
        } else if (any_discard) {
            auto wait = make_node(ExecutionGraphNodeKind::synchronization_region);
            wait.synchronization =
                ExecutionSynchronizationKind::backend_stream_wait;
            wait.work_unit_kind.reset();
            wait.work_units = 0U;
            append_node(std::move(wait));
        }
    }

    const AutoregressiveExecutionState state{
        capabilities.kv_storage,
        plan.kv.page_tokens,
    };
    return finalize_execution_graph(
        plan.backend,
        ExecutionGraphBindingKind::air_executable,
        WorkloadPhysicalInvocation{std::move(invocation)},
        state,
        std::move(nodes));
}

std::string serialize_execution_graph(const ExecutionGraph& graph) {
    std::ostringstream out;
    out << "identity=" << graph.identity() << '\n';
    out << canonical_graph_body(graph);
    return out.str();
}

} // namespace air
