#include "air/execution.hpp"

#include <algorithm>

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

OperationImplementationFamily implementation_family(
    QualifiedOperationSite site) noexcept {
    switch (site) {
    case QualifiedOperationSite::prefill_transformer_block_linear:
    case QualifiedOperationSite::decode_transformer_block_linear:
    case QualifiedOperationSite::decode_output_projection:
        return OperationImplementationFamily::linear;
    case QualifiedOperationSite::prefill_attention:
    case QualifiedOperationSite::decode_attention:
        return OperationImplementationFamily::attention;
    }
    return OperationImplementationFamily::linear;
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

} // namespace air
